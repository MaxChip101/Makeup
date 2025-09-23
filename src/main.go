package main

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
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
	Condition   int             `json:"condition"`
}

type Profiles struct {
	Public  []Profile `json:"public"`
	Private []Profile `json:"private"`
}

type Build struct {
	Profiles Profiles `json:"profiles"`
}

type Makeup struct {
	Version string `json:"version"`
}

type Configurations struct {
	Project Project `json:"project"`
	Build   Build   `json:"build"`
	Makeup  Makeup  `json:"makeup"`
}

// preset structs
type Preset struct {
	Profiles Profiles `json:"profiles"`
	Makeup   Makeup   `json:"makeup"`
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
	os.WriteFile(filepath.Join(".makeup", "config.json"), data, 0644)
	return err
}

func ReadConfigFile(configs *Configurations) error {
	data, err := os.ReadFile(filepath.Join(".makeup", "config.json"))
	if err != nil {
		return err
	}

	json.Unmarshal(data, configs)
	return nil
}

func CheckMakeup() (bool, error) {
	_, err := os.Stat(".makeup")
	if os.IsNotExist(err) {
		return false, nil
	} else if err != nil {
		return false, err
	}
	return true, nil
}

func MakeupInitialized() (bool, error) {
	dir, err := os.UserHomeDir()
	if err != nil {
		return false, err
	}
	_, err = os.Stat(filepath.Join(dir, ".makeup"))
	if os.IsNotExist(err) {
		return false, nil
	} else if err != nil {
		return false, err
	}
	return true, nil
}

func ReadPreset(preset_name string, preset *Preset) error {
	dir, err := os.UserHomeDir()
	if err != nil {
		return err
	}

	data, err := os.ReadFile(filepath.Join(dir, ".makeup", "presets", preset_name+".json"))
	if err != nil {
		return err
	}

	json.Unmarshal(data, preset)
	return nil
}

func InitializeMakeupConfig(preset_name string) {
	exists, err := CheckMakeup()
	if err != nil {
		log.Fatal(err)
	} else if exists {
		fmt.Println("Makeup already exists here")
		os.Exit(0)
	}

	err = os.MkdirAll(filepath.Join(".makeup", "cache"), 0755)
	if err != nil {
		log.Fatal(err)
	}
	_, err = os.Create(filepath.Join(".makeup", "config.json"))
	if err != nil {
		log.Fatal(err)
	}

	var configs Configurations

	if preset_name == "none" {
		configs.Build.Profiles.Public = []Profile{
			{Name: ""},
		}
		configs.Build.Profiles.Private = []Profile{
			{Name: ""},
		}
	} else {
		var preset Preset
		err = ReadPreset(preset_name, &preset)
		if err != nil {
			log.Fatal(err)
		}

		configs.Build.Profiles = preset.Profiles
	}

	configs.Makeup.Version = VERSION

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
		files, err := GetFiles(rule.Directory, rule.Regex, rule.Format, rule.Cache)
		if err != nil {
			return nil, err
		}
		command := Command{command: pattern[0]}
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
		wildcard_files, err := GetFiles(rule.WildCardDirectory, rule.WildCardRegex, rule.WildCardFormat, rule.Cache)
		if err != nil {
			return nil, err
		}
		mapped_files, err := GetFiles(rule.WildCardDirectory, rule.WildCardRegex, rule.MapFormat, false)
		if err != nil {
			return nil, err
		}

		for index := range wildcard_files {
			command := Command{command: pattern[0]}
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
			commands = append(commands, command)
		}
	default:
		command := Command{command: pattern[0]}
		command.args = append(command.args, pattern...)
		commands = append(commands, command)
	}

	return commands, nil
}

func GetFiles(directory string, pattern string, format string, cache bool) ([]string, error) {
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
		good := true

		if cache {
			path := filepath.Join(directory, entry.Name())
			good, err = CheckCache(path)
			if err != nil {
				return nil, err
			}
			if good {
				err := CacheFile(path)
				if err != nil {
					return nil, err
				}
			}
		}
		if regex.MatchString(entry.Name()) && good {
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
	// \* is a regular *
	// \- is a regular -
	// \\ is a regular \

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
		case "\\":
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
	err := os.RemoveAll(filepath.Join(".makeup", "cache"))
	if err != nil {
		log.Fatal(err)
	}
	os.Mkdir(filepath.Join(".makeup", "cache"), 0755)
}

func CheckCache(path string) (bool, error) {
	/*
		// check if file is the same as the cache

		cache_hash := sha256.Sum256([]byte(path))
		cache_name := hex.EncodeToString(cache_hash[:])
		cache_full := filepath.Join("./.makeup/cache", cache_name)

		_, err := os.Stat(cache_full)

		if os.IsNotExist(err) {
			return true, nil
		} else if err != nil {
			return false, err
		}

		cache_content, err := os.ReadFile(cache_full)
		if err != nil {
			return false, err
		}

		file_content, err := os.ReadFile(path)
		if err != nil {
			return false, err
		}

		file_hash := sha256.Sum256(file_content)
		result := slices.Compare(cache_content, []byte(file_hash[:]))
		if result == 0 {
			return true, nil
		}
		return false, nil
	*/
	return true, nil
}

func CacheFile(path string) error {
	/* tallen please help me on ts
	// create file in .makeup/cache that corresponds to the file to check
	cache_hash := sha256.Sum256([]byte(path))
	cache_name := hex.EncodeToString(cache_hash[:])
	cache_full := filepath.Join("./.makeup/cache", cache_name)

	file_content, err := os.ReadFile(path)
	if err != nil {
		return err
	}

	encrypted_content := sha256.Sum256(file_content)
	encrypted_content_string := hex.EncodeToString(encrypted_content[:])

	os.WriteFile(cache_full, []byte(encrypted_content_string), 0644)
	return nil
	*/
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
		//fmt.Println(command.command)
		//fmt.Println(command.args)
		cmd := exec.Command(command.command, command.args[1:]...)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr

		err = cmd.Run()
		if err != nil {
			exitError, ok := err.(*exec.ExitError)
			if !ok {
				log.Fatal("Something went wrong")
			}
			if exitError.ExitCode() != profile.Condition {
				log.Fatalf("Condition not met on \"%s\"", profile.Name)
			}
		} else if profile.Condition != 0 {
			log.Fatalf("Condition not met on \"%s\"", profile.Name)
		}

	}

	if profile.NextProfile == "" {
		os.Exit(0)
	}
	ExecuteProfile(profile.NextProfile, false)
}

func Help() {
	fmt.Println("makeup init - initializes the preset directory for makeup\nmakeup new <preset name> - creates new makeup configuration directory with the preset in your current directory\nmakeup clear - clears cache\nmakeup exec <public profile> - executes a public profile in the makeup configuration json")
}

func InitializeMakeup() {
	exists, err := MakeupInitialized()
	if err != nil {
		log.Fatal(err)
	} else if exists {
		fmt.Println("Makeup is already initialized at ~/.makeup")
		os.Exit(0)
	}
	dir, err := os.UserHomeDir()
	if err != nil {
		log.Fatal(err)
	}
	err = os.MkdirAll(filepath.Join(dir, ".makeup", "presets"), 0755)
	if err != nil {
		log.Fatal(err)
	}

}

func main() {
	args := os.Args

	// "makeup"
	if len(args) < 2 {
		Help()
		return
	}

	var binary_index int

	binary, err := os.Executable()
	if err != nil {
		log.Fatal(err)
	}

	for index, arg := range args {
		if arg == binary {
			binary_index = index
		}
	}

	// "makeup <argument>"
	switch args[binary_index+1] {
	case "init":
		InitializeMakeup()
	case "new":
		preset := "none"
		if len(args[binary_index:]) == 3 {
			preset = args[binary_index+2]
		}
		InitializeMakeupConfig(preset)
	case "clear":
		exists, err := CheckMakeup()
		if err != nil {
			log.Fatal(err)
		} else if !exists {
			fmt.Println("Makeup is not initialized here. Type 'makeup new <preset>' to initialize makeup")
			os.Exit(0)
		}
		ClearCache()
	case "exec":
		exists, err := CheckMakeup()
		if err != nil {
			log.Fatal(err)
		} else if !exists {
			fmt.Println("Makeup is not initialized here. Type 'makeup new <preset>' to initialize makeup")
			os.Exit(0)
		} else if len(args[binary_index:]) < 3 {
			fmt.Println("Type 'makeup exec <profile name>' to execute a profile")
			os.Exit(0)
		}
		ExecuteProfile(args[binary_index+2], true)
	default:
		Help()
	}
}
