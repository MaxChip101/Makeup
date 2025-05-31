package main

import (
	"fmt"
	"os"
)

func makeup_help() {
	fmt.Println("I see you need help")
}

func makeup_config_details() {
	fmt.Println("details")
}

func makeup_new_config() {
	err := os.Mkdir(".makeup", 0755)
	if err != nil {
		fmt.Println("Makeup is already initialized here")
		return
	}

	os.Create(".makeup/cache")
	os.Create(".makeup/config.json")

	content := []byte("{\n\"src-dir\": \"./src\",\n\"bin-dir\": \"./bin\",\n\"bin-name\": \"bin_name\",\n\"compiler-options\": \"\",\n\"run-options\": \"\",\n\"debug-options\": \"\",\n\"compiler\": \"go build\",\n\"runner\": \"go run\",\n\"debugger\": \"\"\n}")
	os.WriteFile(".makeup/config.json", content, 0644)
}

func main() {
	args := os.Args

	if len(args) < 2 {
		fmt.Println("Type 'makeup help' for a list of commands")
	}

	for arg := 0; arg < len(args); arg++ {
		if args[arg] == "help" || args[arg] == "--help" || args[arg] == "-h" {
			makeup_help()
			return
		} else if args[arg] == "details" {
			makeup_config_details()
			return
		} else if args[arg] == "new" {
			makeup_new_config()
			return
		}
	}
}
