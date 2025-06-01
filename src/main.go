package main

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

const VERSION string = "0.0.1"

func makeup_exists() (exists bool) {
	_, err := os.Stat(".makeup")
	if err == nil {
		return true
	}

	return false
}

func check_makeup() (all_good bool) {
	version_good := false
	cache_good := false
	config_good := false

	_, err1 := os.Stat(".makeup/version")
	if err1 == nil {
		version_good = true
	}

	_, err2 := os.Stat(".makeup/cache")
	if err2 == nil {
		cache_good = true
	}

	_, err3 := os.Stat(".makeup/config.json")
	if err3 == nil {
		config_good = true
	}

	if version_good && cache_good && config_good {
		return true
	}

	return false
}

func check_version() (up_to_date bool) {
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

func makeup_fix() {

}

func makeup_reset() {
	if !makeup_exists() {
		fmt.Println("Makeup does not exist in this directory, type 'makeup new' to create a makeup configuration")
		return
	}

	if !check_makeup() {
		fmt.Println("There is an issue in the Makeup directory, type 'makeup fix' to hopefully fix the issue")
		return
	}

	content := []byte("{\n\"src-dir\": \"\",\n\"bin-dir\": \"\",\n\"bin-name\": \"\",\n\"compiler-options\": \"\",\n\"run-options\": \"\",\n\"debug-options\": \"\",\n\"compiler\": \"\",\n\"runner\": \"\",\n\"debugger\": \"\"\n}")
	os.WriteFile(".makeup/config.json", content, 0644)
}

func clear_cache() {
	if !makeup_exists() {
		fmt.Println("Makeup does not exist in this directory, type 'makeup new' to create a makeup configuration")
		return
	}

	if !check_makeup() {
		fmt.Println("There is an issue in the Makeup directory, type 'makeup fix' to hopefully fix the issue")
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

func makeup_help() {
	fmt.Println("I see you need help")
}

func makeup_config_details() {
	fmt.Println("details")
}

func makeup_new_config(name string) {
	err := os.Mkdir(".makeup", 0755)
	if err != nil {
		fmt.Println("Makeup is already initialized here")
		return
	}

	os.Create(".makeup/cache")
	os.Create(".makeup/version")
	os.Create(".makeup/config.json")

	os.WriteFile(".makeup/version", []byte(VERSION), 0644)

	content := []byte("{\n\"src-dir\": \"./src\",\n\"bin-dir\": \"./bin\",\n\"bin-name\": \"" + name + "\",\n\"compiler-options\": \"\",\n\"run-options\": \"\",\n\"debug-options\": \"\",\n\"compiler\": \"go build\",\n\"runner\": \"go run\",\n\"debugger\": \"\"\n}")
	os.WriteFile(".makeup/config.json", content, 0644)
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
		makeup_config_details()

		return
	} else if args[1] == "new" {
		if len(args) < 3 {
			cwd, err := os.Getwd()
			if err != nil {
				log.Fatal(err)
			}
			makeup_new_config(filepath.Base(cwd))
			return
		}
		makeup_new_config(args[2])
		return
	} else if args[1] == "set" {
		if len(args) < 4 {
			fmt.Println("Missing arguments, type 'makeup help' for a list of commands")
		}
		// set json variables
	} else if args[1] == "fix" {
		makeup_fix()
		return
	} else if args[1] == "reset" {
		makeup_reset()
		return
	} else if args[1] == "clear" {
		clear_cache()
		return
	} else if args[1] == "destroy" {
		makeup_destroy()
		return
	} else if args[1] == "run" {
		// run the project
	} else if args[1] == "debug" {
		// make a debug build for the project
	} else if args[1] == "build" {
		// build the project
	}
}
