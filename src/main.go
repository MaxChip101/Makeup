package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
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
	Name            string   `json:"name"`
	Compiler        string   `json:"compiler"`
	Objects         bool     `json:"objects"`
	File_Extensions []string `json:"file_extensions"`
	Default_Flags   []string `json:"default_flags"`
}

type Paths struct {
	Source   string   `json:"source"`
	Target   string   `json:"target"`
	Excluded []string `json:"excluded"`
}

type Cache struct {
	Enabled     bool     `json:"enabled"`
	Directories []string `json:"directories"`
}

type Build struct {
	Target_Name string   `json:"target_name"`
	Libraries   []string `json:"libraries"`
}

type Release struct {
	Compile_Flags []string `json:"compile_flags"`
}

type Debug struct {
	Compile_Flags []string `json:"compile_flags"`
}

type Profiles struct {
	Release Release `json:"release"`
	Debug   Debug   `json:"debug"`
}

type Makeup struct {
	Version string `json:"version"`
	Debug   bool   `json:"debug"`
}

type Configs struct {
	Project  Project  `json:"project"`
	Language Language `json:"language"`
	Paths    Paths    `json:"paths"`
	Cache    Cache    `json:"cache"`
	Build    Build    `json:"build"`
	Profiles Profiles `json:"profiles"`
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
	// update to check for verison in the json file
	content_bytes, err := os.ReadFile(".makeup/version")
	if err != nil {
		log.Fatal(err)
	}
	content := string(content_bytes)

	this_version := strings.Split(VERSION, ".")
	project_version := strings.Split(content, ".")

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

func makeup_build(release bool) {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	data := read_json(".makeup/config.json")

	var args []string

	entries, err := os.ReadDir(data.Source_path)

	if err != nil {
		log.Fatal(err)
	}

	final_ouput_string := data.Output_path + "/" + data.Binary_name
	compile_flags := strings.Split(data.Compile_flags, " ")

	args = append(args, compile_flags...)
	args = append(args, "-o", final_ouput_string)

	for _, entry := range entries {
		if !entry.IsDir() {
			args = append(args, data.Source_path+"/"+entry.Name())
		}
	}

	cmd := exec.Command(data.Compiler, args...)

	output, err := cmd.CombinedOutput()

	if err != nil {
		log.Fatal(err)
	}
	fmt.Println(string(output))

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
			makeup_build(true)
			return
		}
		makeup_build(false)
		return
	}
}
