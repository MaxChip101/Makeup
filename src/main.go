package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"slices"
	"strconv"
	"strings"
)

const VERSION string = "0.0.1"

type Project struct {
	Version     string `json:"version"`
	Name        string `json:"name"`
	Description string `json:"description"`
	License     string `json:"license"`
}

type Language struct {
	Name                  string   `json:"name"`
	Windows_Compiler      string   `json:"windows_compiler"`
	Linux_Compiler        string   `json:"linux_compiler"`
	Mac_Compiler          string   `json:"mac_compiler"`
	Windows_Linker        string   `json:"windows_linker"`
	Linux_Linker          string   `json:"linux_linker"`
	Mac_Linker            string   `json:"mac_linker"`
	Generate_Object_Files bool     `json:"generate_object_files"`
	File_Extensions       []string `json:"file_extensions"`
	Default_Compile_Flags []string `json:"default_compile_flags"`
	Default_Link_Flags    []string `json:"default_link_flags"`
	Library_Prefix        string   `json:"library_prefix"`
	Compile_Prefix        string   `json:"compile_prefix"`
	Link_Template         string   `json:"link_template"`
	Compile_Template      string   `json:"compile_template"`
}

type Paths struct {
	Source   []string `json:"source"`
	Artifact string   `json:"artifact"`
	Exclude  []string `json:"exclude"`
}

type Cache struct {
	Enabled     bool     `json:"enabled"`
	Directories []string `json:"directories"`
}

type Build struct {
	Windows     bool     `json:"windows"`
	Linux       bool     `json:"linux"`
	Mac         bool     `json:"mac"`
	Target_Name string   `json:"target_name"`
	Libraries   []string `json:"libraries"`
}

type Platform struct {
	Compile_Flags []string `json:"compile_flags"`
	Link_Flags    []string `json:"link_flags"`
}

type Profile struct {
	Windows Platform `json:"windows"`
	Linux   Platform `json:"linux"`
	Mac     Platform `json:"mac"`
}

type Profiles struct {
	Release Profile `json:"release"`
	Debug   Profile `json:"debug"`
}

type Hooks struct {
	Pre_Build  []string `json:"pre_build"`
	Post_Build []string `json:"post_build"`
}

type Makeup struct {
	Version string `json:"version"`
}

type Configs struct {
	Project  Project  `json:"project"`
	Language Language `json:"language"`
	Paths    Paths    `json:"paths"`
	Cache    Cache    `json:"cache"`
	Build    Build    `json:"build"`
	Profiles Profiles `json:"profiles"`
	Hooks    Hooks    `json:"hooks"`
	Makeup   Makeup   `json:"makeup"`
}

func makeup_exists() (exists bool) {
	_, err := os.Stat(".makeup")
	if err == nil {
		return true
	}

	fmt.Println("Makeup does not exist in this directory, type 'makeup new' to create a makeup configuration")
	return false
}

func check_makeup(print_error bool) (all_good bool) {
	cache_good := false
	config_good := false

	_, err2 := os.Stat(".makeup/cache")
	if err2 == nil {
		cache_good = true
	}

	_, err3 := os.Stat(".makeup/config.json")
	if err3 == nil {
		config_good = true
	}

	if cache_good && config_good {
		return true
	}

	if print_error {
		fmt.Println("There is an issue in the Makeup directory, type 'makeup fix' to hopefully fix the issue")
	}

	return false
}

func check_version() (up_to_date bool) {
	data := read_json(".makeup/config.json")
	makeup_version := data.Makeup.Version

	this_version := strings.Split(VERSION, ".")
	project_version := strings.Split(makeup_version, ".")

	up_to_date = false

	for i := range 3 {
		project_ver_num, err := strconv.Atoi(project_version[i])
		if err != nil {
			log.Fatal(err)
		}
		this_ver_num, err := strconv.Atoi(this_version[i])
		if err != nil {
			log.Fatal(err)
		}

		if project_ver_num >= this_ver_num {
			up_to_date = true
		}
	}

	if up_to_date {
		return true
	}

	return false
}

func read_json(path string) (content Configs) {
	var data Configs

	readContent, err := os.ReadFile(path)
	if err != nil {
		log.Fatal(err)
	}

	err = json.Unmarshal(readContent, &data)
	if err != nil {
		log.Fatal(err)
	}
	return data
}

func write_json(path string, data Configs) {
	content, err := json.MarshalIndent(data, "", "	")
	if err != nil {
		log.Fatal(err)
	}
	os.WriteFile(path, content, 0644)
}

func makeup_fix() {
	if !makeup_exists() {
		return
	}
	if check_makeup(false) {
		fmt.Println("Makeup is all good")
		return
	}

	_, err1 := os.Stat(".makeup/cache")
	if err1 != nil {
		os.Create(".makeup/cache")
	}
	_, err2 := os.Stat(".makeup/config.json")
	if err2 != nil {
		os.Create(".makeup/config.json")
	}
}

func clear_cache() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	os.WriteFile(".makeup/cache", []byte{0}, 0644)
}

func makeup_remove(y_flag_enabled bool) {
	if !makeup_exists() {
		return
	}

	var response string

	if !y_flag_enabled {
		fmt.Print("Are you sure? [Y/n] ")
		fmt.Scanf("%s", &response)
		if strings.ToLower(response) != "y" {
			return
		}
	}

	err := os.RemoveAll(".makeup")
	if err != nil {
		log.Fatal(err)
	}
}

func recursive_file_thing(directory string, data Configs) (files []string) {

	entries, err := os.ReadDir(directory)

	if err != nil {
		log.Fatal(err)
	}
	for _, entry := range entries {
		if !slices.Contains(data.Paths.Exclude, entry.Name()) && slices.Contains(data.Language.File_Extensions, entry.Name()) {
			if entry.IsDir() {
				files = append(files, recursive_file_thing(directory+"/"+entry.Name(), data)...)
			} else {
				files = append(files, directory+"/"+entry.Name())
			}
		}
	}
	return files
}

func initialize_directories(data Configs) {
	os.Mkdir(data.Paths.Artifact, 0644)
	if data.Build.Windows {
		os.MkdirAll(data.Paths.Artifact+"/win/debug", 0644)
		os.Mkdir(data.Paths.Artifact+"/win/release", 0644)
		if data.Language.Generate_Object_Files {
			os.Mkdir(data.Paths.Artifact+"/win/release/objects", 0644)
			os.Mkdir(data.Paths.Artifact+"/win/debug/objects", 0644)
		}
	}
	if data.Build.Linux {
		os.MkdirAll(data.Paths.Artifact+"/lin/debug", 0644)
		os.Mkdir(data.Paths.Artifact+"/lin/release", 0644)
		if data.Language.Generate_Object_Files {
			os.Mkdir(data.Paths.Artifact+"/lin/release/objects", 0644)
			os.Mkdir(data.Paths.Artifact+"/lin/debug/objects", 0644)
		}
	}
	if data.Build.Mac {
		os.MkdirAll(data.Paths.Artifact+"/mac/debug", 0644)
		os.Mkdir(data.Paths.Artifact+"/mac/release", 0644)
		if data.Language.Generate_Object_Files {
			os.Mkdir(data.Paths.Artifact+"/mac/release/objects", 0644)
			os.Mkdir(data.Paths.Artifact+"/mac/debug/objects", 0644)
		}
	}
}

func linux_build(release bool, data Configs, input_files []string) {
	var args []string
	var target string
	var object_directory string
	if release {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/release/objects/"
		}
		target = data.Paths.Artifact + "/lin/release/" + data.Build.Target_Name
	} else {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/debug/objects/"
		}
		target = data.Paths.Artifact + "/lin/debug/" + data.Build.Target_Name
	}

	fmt.Println(object_directory)

	compile_template := strings.Split(data.Language.Compile_Template, " ")

	var libraries []string

	for _, library := range data.Build.Libraries {
		libraries = append(libraries, data.Language.Library_Prefix+library)
	}

	if data.Language.Generate_Object_Files {

		for i := 0; i < len(input_files); i++ {
			for _, arg := range compile_template {
				switch arg {
				case "{input_files}":
					args = append(args, input_files[0])
				case "{flags}":
					args = append(args, data.Language.Default_Compile_Flags...)
					if release {
						args = append(args, data.Profiles.Release.Linux.Compile_Flags...)
					} else {
						args = append(args, data.Profiles.Debug.Linux.Compile_Flags...)
					}
				case "{target}":
					args = append(args, target)
				default:
					if arg != "{compiler}" {
						args = append(args, arg)
					}
				}
			}
			cmd := exec.Command(data.Language.Linux_Compiler, args...)
			output, err := cmd.CombinedOutput()

			if err != nil {
				fmt.Println(string(output))
				log.Fatal(err)
			}

			fmt.Println(string(output))
		}
	} else {
		for _, arg := range compile_template {
			switch arg {
			case "{input_files}":
				args = append(args, input_files...)
			case "{flags}":
				args = append(args, data.Language.Default_Compile_Flags...)
				if release {
					args = append(args, data.Profiles.Release.Linux.Compile_Flags...)
				} else {
					args = append(args, data.Profiles.Debug.Linux.Compile_Flags...)
				}
			case "{target}":
				args = append(args, target)
			default:
				if arg != "{compiler}" {
					args = append(args, arg)
				}
			}
		}
		cmd := exec.Command(data.Language.Linux_Compiler, args...)
		output, err := cmd.CombinedOutput()

		if err != nil {
			fmt.Println(string(output))
			log.Fatal(err)
		}

		fmt.Println(string(output))
	}

}

func windows_build(release bool, data Configs, input_files []string) {
	var args []string
	var target string
	var object_directory string
	if release {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/release/objects/"
		}
		target = data.Paths.Artifact + "/lin/release/" + data.Build.Target_Name
	} else {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/debug/objects/"
		}
		target = data.Paths.Artifact + "/lin/debug/" + data.Build.Target_Name
	}

	fmt.Println(object_directory)

	compile_template := strings.Split(data.Language.Compile_Template, " ")

	var libraries []string

	for _, library := range data.Build.Libraries {
		libraries = append(libraries, data.Language.Library_Prefix+library)
	}

	for _, arg := range compile_template {
		switch arg {
		case "{input_files}":
			args = append(args, input_files...)
		case "{flags}":
			args = append(args, data.Language.Default_Compile_Flags...)
			if release {
				args = append(args, data.Profiles.Release.Linux.Compile_Flags...)
			} else {
				args = append(args, data.Profiles.Debug.Linux.Compile_Flags...)
			}
		case "{target}":
			args = append(args, target)
		default:
			args = append(args, arg)
		}
	}
	cmd := exec.Command(data.Language.Linux_Compiler, args...)

	output, err := cmd.CombinedOutput()

	if err != nil {
		log.Fatal(err)
	}

	fmt.Println(string(output))
}

func mac_build(release bool, data Configs, input_files []string) {
	var args []string
	var target string
	var object_directory string
	if release {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/release/objects/"
		}
		target = data.Paths.Artifact + "/lin/release/" + data.Build.Target_Name
	} else {
		if data.Language.Generate_Object_Files {
			object_directory = data.Paths.Artifact + "/lin/debug/objects/"
		}
		target = data.Paths.Artifact + "/lin/debug/" + data.Build.Target_Name
	}

	fmt.Println(object_directory)

	compile_template := strings.Split(data.Language.Compile_Template, " ")

	var libraries []string

	for _, library := range data.Build.Libraries {
		libraries = append(libraries, data.Language.Library_Prefix+library)
	}

	for _, arg := range compile_template {
		switch arg {
		case "{input_files}":
			args = append(args, input_files...)
		case "{flags}":
			args = append(args, data.Language.Default_Compile_Flags...)
			if release {
				args = append(args, data.Profiles.Release.Linux.Compile_Flags...)
			} else {
				args = append(args, data.Profiles.Debug.Linux.Compile_Flags...)
			}
		case "{target}":
			args = append(args, target)
		default:
			args = append(args, arg)
		}
	}
	cmd := exec.Command(data.Language.Linux_Compiler, args...)

	output, err := cmd.CombinedOutput()

	if err != nil {
		log.Fatal(err)
	}

	fmt.Println(string(output))
}

func makeup_build(platform string, release bool) {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	data := read_json(".makeup/config.json")

	var source []string

	for _, dir := range data.Paths.Source {
		source = append(source, recursive_file_thing(dir, data)...)
	}

	initialize_directories(data)

	switch platform {
	case "windows":
		windows_build(release, data, source)
	case "linux":
		linux_build(release, data, source)
	case "mac":
		mac_build(release, data, source)
	}

}

func makeup_clean() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

}

func makeup_help() {
	fmt.Println("I see you need help")
}

func makeup_new(name string) {
	err := os.Mkdir(".makeup", 0755)
	if err != nil {
		fmt.Println("Makeup is already initialized here")
		return
	}

	os.Create(".makeup/cache")
	os.Create(".makeup/config.json")

	data := Configs{}

	write_json(".makeup/config.json", data)
}

func main() {
	args := os.Args

	if len(args) < 2 {
		fmt.Println("Type 'makeup help' for a list of commands")
		return
	}

	if args[1] == "help" || args[1] == "--help" || args[1] == "-h" {
		makeup_help()
		return
	} else if args[1] == "new" {
		if len(args) < 3 {
			cwd, err := os.Getwd()
			if err != nil {
				log.Fatal(err)
			}
			makeup_new(filepath.Base(cwd))
			return
		}
		makeup_new(args[2])
		return
	} else if args[1] == "fix" {
		makeup_fix()
		return
	} else if args[1] == "clear" {
		clear_cache()
		return
	} else if args[1] == "remove" {
		if len(args) == 3 && args[2] == "-y" {
			makeup_remove(true)
			return
		}
		makeup_remove(false)
		return
	} else if args[1] == "build" {
		if len(args) == 3 && (args[2] == "--release" || args[2] == "-r") {
			makeup_build("linux", true)
			return
		}
		makeup_build("linux", false)
		return
	}
}
