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

type Configs struct {
	Version       string `json:"version"`
	Project_name  string `json:"project-name"`
	Source_path   string `json:"source-path"`
	Output_path   string `json:"output-path"`
	Binary_name   string `json:"binary-name"`
	Compile_flags string `json:"compile-flags"`
	Run_flags     string `json:"run-flags"`
	Debug_flags   string `json:"debug-flags"`
	Compiler      string `json:"compiler"`
	Runner        string `json:"runner"`
	Debugger      string `json:"debugger"`
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

func read_json() (content Configs) {
	var data Configs

	readContent, err := os.ReadFile(".makeup/config.json")
	if err != nil {
		log.Fatal(err)
	}

	err = json.Unmarshal(readContent, &data)
	if err != nil {
		log.Fatal(err)
	}
	return data
}

func write_json(data Configs) {
	content, err := json.MarshalIndent(data, "", "	")
	if err != nil {
		log.Fatal(err)
	}
	os.WriteFile(".makeup/config.json", content, 0644)
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

func makeup_destroy() {
	if !makeup_exists() {
		fmt.Println("Makeup does not exist in this directory, type 'makeup new' to create a makeup configuration")
		return
	}

	err := os.RemoveAll(".makeup")
	if err != nil {
		log.Fatal(err)
	}
}

func makeup_set(variable string, value string) {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	data := read_json()

	if variable == "project-name" {
		data.Project_name = value
	} else if variable == "source-path" {
		data.Source_path = value
	} else if variable == "output-path" {
		data.Output_path = value
	} else if variable == "binary-name" {
		data.Binary_name = value
	} else if variable == "compiler-flags" {
		data.Compile_flags = value
	} else if variable == "run-flags" {
		data.Run_flags = value
	} else if variable == "debug-flags" {
		data.Debug_flags = value
	} else if variable == "compiler" {
		data.Compiler = value
	} else if variable == "runner" {
		data.Runner = value
	} else if variable == "debugger" {
		data.Debugger = value
	} else {
		fmt.Println("That variable does not exist/cannot be changed")
		return
	}

	write_json(data)
}

func makeup_build() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	data := read_json()

	var files string

	entries, err := os.ReadDir(data.Source_path)

	if err != nil {
		log.Fatal(err)
	}

	for _, entry := range entries {
		if !entry.IsDir() {
			files += data.Source_path + "/" + entry.Name() + " "
		}
	}

	final_ouput_string := data.Output_path + "/" + data.Binary_name

	cmd := exec.Command(data.Compiler, data.Compile_flags, "-o", final_ouput_string, files)
	output, err := cmd.CombinedOutput()

	if err != nil {
		log.Fatal(err)
	}
	fmt.Println(string(output))

}

func makeup_debug() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}
}

func makeup_run() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}
}

func makeup_help() {
	fmt.Println("I see you need help")
}

func makeup_details() {
	if !makeup_exists() || !check_makeup(true) {
		return
	}

	fmt.Println("details")
}

func makeup_new(name string) {
	err := os.Mkdir(".makeup", 0755)
	if err != nil {
		fmt.Println("Makeup is already initialized here")
		return
	}

	os.Create(".makeup/cache")
	os.Create(".makeup/config.json")

	data := Configs{Version: VERSION, Project_name: name, Source_path: "src", Output_path: "bin", Binary_name: name, Compile_flags: "", Run_flags: "", Debug_flags: "", Compiler: "", Runner: "", Debugger: ""}

	write_json(data)
}

func main() {
	args := os.Args

	if len(args) < 2 {
		fmt.Println("Type 'makeup help' for a list of commands")
	}

	if args[1] == "help" || args[1] == "--help" || args[1] == "-h" {
		makeup_help()
		return
	} else if args[1] == "details" {
		makeup_details()

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
	} else if args[1] == "set" {
		if len(args) < 4 {
			fmt.Println("Missing arguments, type 'makeup help' for a list of commands")
		}
		makeup_set(args[2], args[3])
	} else if args[1] == "fix" {
		makeup_fix()
		return
	} else if args[1] == "clear" {
		clear_cache()
		return
	} else if args[1] == "destroy" {
		makeup_destroy()
		return
	} else if args[1] == "run" {
		makeup_run()
	} else if args[1] == "debug" {
		makeup_debug()
	} else if args[1] == "build" {
		makeup_build()
	}
}
