package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"regexp"
	"strings"
)

const VERSION string = "0.0.1"

// Json Structure
type Project struct {
	Version     string `json:"version"`
	Name        string `json:"name"`
	Description string `json:"description"`
	License     string `json:"license"`
}

type Profile struct {
	Name        string          `json:"name"`
	Type        string          `json:"type"`
	RuleSet     json.RawMessage `json:"rule-set"`
	Template    string          `json:"template"`
	NextProfile string          `json:"next-profile"`
	Conditions  []int8          `json:"conditions"`
}

type Profiles struct {
	Public  []Profile `json:"public"`
	Private []Profile `json:"private"`
}

type Build struct {
	Profiles Profiles `json:"profiles"`
}

type Makeup struct {
	Version        string `json:"version"`
	DefaultProfile string `json:"default-profile"`
}

type Configurations struct {
	Project Project `json:"project"`
	Build   Build   `json:"build"`
	Makeup  Makeup  `json:"makeup"`
}

// Profile Rule Sets
type WildCardRuleSet struct {
	Directory string `json:"directory"`
	Regex     string `json:"regex"`
	Format    string `json:"format"`
	Cache     bool   `json:"cache"`
}

type WildCardMapRuleSet struct {
	WildCardDirectory string `json:"wildcard-dir"`
	WildCardRegex     string `json:"wildcard-regex"`
	WildCardFormat    string `json:"wildcard-format"`
	Cache             bool   `json:"cache"`
	MapFormat         string `json:"map-format"`
	// this should be able to get a list of files from a directory and be able to use the compiler to map the .o files
}

// Other Structs
type Command struct {
	command string
	args    []string
}

func WriteConfigFile(configs Configurations) error {
	data, err := json.MarshalIndent(configs, "", "\t")
	if err != nil {
		return err
	}
	os.WriteFile("./.makeup/config.json", data, 0644)
	return err
}

func ReadConfigFile(configs *Configurations) error {
	data, err := os.ReadFile("./.makeup/config.json")
	if err != nil {
		return err
	}

	json.Unmarshal(data, configs)
	return nil
}

func CheckMakeup() (bool, error) {
	_, err := os.Stat("./.makeup")
	if os.IsNotExist(err) {
		return false, nil
	} else if err != nil {
		return false, err
	}
	return true, nil
}

func InitializeMakeup(preset string) {
	exists, err := CheckMakeup()
	if err != nil {
		log.Fatal(err)
	} else if exists {
		fmt.Println("Makeup already exists here")
	}

	err = os.MkdirAll("./.makeup/cache", 0755)
	if err != nil {
		log.Fatal(err)
	}
	_, err = os.Create("./.makeup/config.json")
	if err != nil {
		log.Fatal(err)
	}

	var configs Configurations
	err = WriteConfigFile(configs)
	if err != nil {
		log.Fatal(err)
	}
}

func GetProfile(name string, public bool) (Profile, error) {
	var config Configurations
	err := ReadConfigFile(&config)
	if err != nil {
		return Profile{}, err
	}

	if public {
		for _, profile := range config.Build.Profiles.Public {
			if profile.Name == name {
				return profile, nil
			}
		}
	} else {
		for _, profile := range config.Build.Profiles.Private {
			if profile.Name == name {
				return profile, nil
			}
		}
	}

	return Profile{}, fmt.Errorf("Profile \"%s\" does not exist", name)
}

func CreateProfileCommand(profile Profile) ([]Command, error) {
	var commands []Command
	pattern := strings.Split(profile.Template, " ")
	switch profile.Type {
	case "wildcard":
		var rule WildCardRuleSet
		err := json.Unmarshal(profile.RuleSet, &rule)
		if err != nil {
			return nil, err
		}
		files, err := GetFiles(rule.Directory, rule.Regex, rule.Format)
		if err != nil {
			return nil, err
		}
		command := Command{command: pattern[0]}
		pattern[0] = ""
		for _, arg := range pattern {
			switch arg {
			case "{wildcard}":
				command.args = append(command.args, files...)
			default:
				command.args = append(command.args, arg)
			} // there might be more variables for template syntax
		}
		commands = append(commands, command)
	case "wildcard+map":
		var rule WildCardMapRuleSet
		err := json.Unmarshal(profile.RuleSet, &rule)
		if err != nil {
			return nil, err
		}
		wildcard_files, err := GetFiles(rule.WildCardDirectory, rule.WildCardRegex, rule.WildCardFormat)
		if err != nil {
			return nil, err
		}
		mapped_files, err := GetFiles(rule.WildCardDirectory, rule.WildCardRegex, rule.MapFormat)
		if err != nil {
			return nil, err
		}
		commands = make([]Command, len(wildcard_files))
		for _, command := range commands {
			command.command = pattern[0]
		}
		pattern[0] = ""

		for index, command := range commands {
			for _, arg := range pattern {
				switch arg {
				case "{wildcard}":
					command.args = append(command.args, wildcard_files[index])
				case "{map}":
					command.args = append(command.args, mapped_files[index])
				default:
					command.args = append(command.args, arg)
				} // there might be more variables for template syntax
			}
		}

	default:
		command := Command{command: pattern[0]}
		pattern[0] = ""
		command.args = append(command.args, pattern...)
		commands = append(commands, command)
	}

	return commands, nil
}

func GetFiles(directory string, pattern string, format string) ([]string, error) {
	entries, err := os.ReadDir(directory)
	if err != nil {
		return nil, err
	}

	regex, err := regexp.Compile(pattern)
	if err != nil {
		return nil, err
	}

	var files []string

	for _, entry := range entries {
		if regex.MatchString(entry.Name()) {
			files = append(files, Formatter(entry.Name(), format))
		}
	}

	return files, nil
}

func FormatAll(inputs []string, format string) []string {
	var final []string
	for _, input := range inputs {
		final = append(final, Formatter(input, format))
	}
	return final
}

func Formatter(input string, format string) string {
	// * is the input text
	// - subtracts 1 character from the end
	// ~* is a regular *
	// ~- is a regular -
	// ~~ is a regular ~
	// i had to change \ to ~ bc of json syntax problems 😑

	var final string

	format_slice := strings.Split(format, "")
	escape := false
	for _, char := range format_slice {
		if escape {
			final += char
			escape = false
			continue
		}

		switch char {
		case "~":
			escape = true
		case "*":
			final += input
		case "-":
			modified := final[:len(final)-1]
			final = modified
		default:
			final += char
		}
	}

	return final
}

func ClearCache() {
	err := os.RemoveAll("./.makeup/cache")
	if err != nil {
		log.Fatal(err)
	}
	os.Mkdir("./.makeup/cache", 0755)
}

func CheckCache(path string) (bool, error) {
	// check if file is the same as the cache
	return true, nil
}

func CacheFile(path string) error {
	// create file in .makeup/cache that corresponds to the file to check
	return nil
}

func ExecuteProfile(profile_name string, public bool) {
	var profile Profile
	var err error
	if public {
		profile, err = GetProfile(profile_name, true)
	} else {
		profile, err = GetProfile(profile_name, false)
	}

	if err != nil {
		log.Fatal(err)
	}

	commands, err := CreateProfileCommand(profile)
	if err != nil {
		log.Fatal(err)
	}

	for _, command := range commands {
		fmt.Println(command.command)
		fmt.Println(command.args)
	}

	/*
		// for loop to execute, see all exit codes and see if they are all valid
		cmd := exec.Command(command, args...)
		var output_buffer bytes.Buffer
		cmd.Stdout = &output_buffer
		cmd.Stderr = &output_buffer

		err = cmd.Run()
		if err != nil {
			exitError, ok := err.(*exec.ExitError)
			if !ok {
				log.Fatal("Something went wrong")
			}
			if exitError.ExitCode() != int(profile.Condition) {
				log.Fatalf("Condition not met on \"%s\"", profile.Name)
			}
		} else if profile.Condition != 0 {
			log.Fatalf("Condition not met on \"%s\"", profile.Name)
		}

		if profile.NextProfile == "" {
			os.Exit(0)
		}

		ExecuteProfile(profile.NextProfile, false)
		// parse command and execute then call the recursive execute private profile function until done or error code that is not in the condition
	*/
}

func Help() {

}

func main() {
	args := os.Args

	// "makeup"
	if len(args) < 2 {
		fmt.Println("Type 'makeup help' for a list of commands")
		return
	}

	// "makeup <argument>"
	switch args[1] {
	case "help":
		subject := "none"
		if len(args) == 3 {
			subject = args[2]
		}
		fmt.Println(subject)
		// makeup help command with subject
	case "new":
		preset := "default"
		if len(args) == 3 {
			preset = args[2]
		}
		InitializeMakeup(preset)
	case "clear": // clear cache folder
		ClearCache()
	case "exec":
		if len(args) < 3 {
			fmt.Println("Type 'makeup exec <profile name>' to execute a profile")
			os.Exit(0)
		}
		ExecuteProfile(args[2], true)
		// execute a profile in the public profiles
	default:
		// makeup help

	}
}
