#include <bits/stdc++.h>
#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Definitions
const char *MAKEUP_VERSION = "0.0.1";

// Flags
const std::string MAKEUP_VERSION_FLAG_SHORT = "-v";
const std::string MAKEUP_VERSION_FLAG = "--version";
const std::string MAKEUP_COMMANDS_FLAG_SHORT = "-h";
const std::string MAKEUP_COMMANDS_FLAG = "--help";
const std::string MAKEUP_DIRECTORY_FLAG_SHORT = "-d";
const std::string MAKEUP_DIRECTORY_FLAG = "--directory";

typedef enum {
    TOKEN_UNKOWN,
    TOKEN_SYMBOL,
    TOKEN_LIT,
    TOKEN_VAR,
    TOKEN_VARREF,
    TOKEN_FUNC,
    TOKEN_FUNCREF,
    TOKEN_MAKEUP_FUNC,
    TOKEN_EOF,
} TokenType;

typedef struct {
    std::string value;
    TokenType type;
    size_t line;
    size_t col;
} Token;

std::map<std::string, std::string> variables;
std::map<std::string, int> functions;
std::map<std::string, std::string> argument_calls;

std::vector<Token> tokenize(std::string content);
std::string exec(const std::string &cmd);
std::string shell_command(std::string input);
void initialize_functions(std::vector<Token> tokens);
void initialize_variables(std::vector<Token> tokens);
void interperet(std::vector<Token> tokens);

int main(int argc, char *argv[]) {
    char cwd[1024];
    std::string tempcwd = getcwd(cwd, sizeof(cwd));
    std::string full_cwd = tempcwd + "/Makeup";

    // check for flags
    if (argc > 1) {
        if (argv[1] == MAKEUP_COMMANDS_FLAG_SHORT ||
            argv[1] == MAKEUP_COMMANDS_FLAG) {
            std::cout << "Options:\n " << MAKEUP_COMMANDS_FLAG_SHORT << ", "
                      << MAKEUP_COMMANDS_FLAG
                      << "\t\tPrints list of commands.\n "
                      << MAKEUP_VERSION_FLAG_SHORT << ", "
                      << MAKEUP_VERSION_FLAG << "\t\tPrints Makeup version.\n "
                      << MAKEUP_DIRECTORY_FLAG_SHORT << ", "
                      << MAKEUP_DIRECTORY_FLAG
                      << "\tPrints the current working directory." << std::endl;
            exit(EXIT_SUCCESS);
        } else if (argv[1] == MAKEUP_VERSION_FLAG ||
                   argv[1] == MAKEUP_VERSION_FLAG_SHORT) {
            std::cout << "Makeup version " << MAKEUP_VERSION << std::endl;
            exit(EXIT_SUCCESS);
        } else if (argv[1] == MAKEUP_DIRECTORY_FLAG ||
                   argv[1] == MAKEUP_DIRECTORY_FLAG_SHORT) {
            std::cout << std::filesystem::current_path() << std::endl;
            exit(EXIT_SUCCESS);
        }
    }

    // check for Makeup file
    std::ifstream makeup_file(full_cwd.c_str());
    if (!makeup_file.is_open()) {
        std::cerr << "Makeup file does not exist in "
                  << std::filesystem::current_path()
                  << ", please make a Makeup file.\ntype `makeup -h` or "
                     "`makeup --help` "
                     "for flags"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string buffer;
    std::string line;
    while (getline(makeup_file, line)) {
        buffer.append(line);
        buffer.push_back('\n');
    }
    makeup_file.close();

    buffer.push_back('\0');

    interperet(tokenize(buffer));

    return (0);
}

std::vector<Token> tokenize(std::string content) {
    size_t index = 0;
    size_t line = 1;
    size_t col = 1;
    bool include_spaces = false;
    bool commented = false;
    TokenType type;
    std::string value;
    std::vector<Token> tokens;
    while (index < content.length() && content[index] != '\0') {
        if (!commented && !value.empty()) {
            tokens.push_back(
                Token{.value = value, .type = type, .line = line, .col = col});
        }

        value.clear();

        if (isalpha(content[index])) {
            type = TOKEN_LIT;
            size_t start = index;
            size_t index2 = 0;

            while ((start + index2 < content.length() &&
                    isalnum(content[start + index2])) ||
                   content[start + index2] == '_') {
                value.push_back(content[start + index2]);
                index2++;
            }
            index += index2;
            col += index2;
        } else {
            if (content[index] == '(' && content[index + 1] == '\"' &&
                !include_spaces) {
                include_spaces = true;
                index += 2;
                col += 2;
            } else if (index + 2 < content.length() && content[index] == '\"' &&
                       content[index + 1] == ')' && !include_spaces) {
                include_spaces = false;
                index += 2;
                col += 2;
                continue;
            } else if (content[index] == '\n') {
                line++;
                col = 0;
                index++;
                commented = false;
            } else if ((content[index] == ' ' || content[index] == '\t') &&
                       !include_spaces) {
                col++;
                index++;
                continue;
            } else if (content[index] == '#') {
                commented = !commented;
                col++;
                index++;
                continue;
            } else {
                col++;
                index++;
            }
            value = content[index];
            type = TOKEN_SYMBOL;
        }
    }
    tokens.push_back(
        Token{.value = "\0", .type = TOKEN_EOF, .line = line, .col = col});

    return (tokens);
}

std::string exec(const std::string &cmd) {
    std::string result;
    char buffer[128];

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "\0";
    }

    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result.append(buffer);
        }
    }

    pclose(pipe);
    return result;
}

std::string shell_command(std::string input) {
    std::string final_string;
    std::string reversed_string;
    for (size_t index = input.length(); index-- > 0;) {
        if (index + 1 < input.length() && input[index] == '!' &&
            input[index + 1] == '(') {
            index += 2;
            size_t index2 = index;
            std::string command = "";
            while (index2 < input.length() && input[index2] != ')') {
                command.push_back(input[index2]);
                index2++;
            }
            std::string value = exec(command);
            for (size_t index3 = value.length(); index3-- > 0;) {
                reversed_string.push_back(value[index3]);
            }
            index = index2;
        } else if (input[index] == ')') {
            continue;
        } else {
            reversed_string.push_back(input[index]);
        }
    }

    for (int index2 = reversed_string.length() - 1; index2 >= 0; index2--) {
        final_string.push_back(reversed_string[index2]);
    }

    return final_string;
}

void initialize_functions(std::vector<Token> tokens) {
    size_t index = 0;
    while (index < tokens.size() && tokens[index].type != TOKEN_EOF) {
        if (index + 1 < tokens.size() && tokens[index].type == TOKEN_LIT &&
            tokens[index + 1].value == "(") {
            functions[tokens[index].value] = index;
        }
        index++;
    }
}

void initialize_variables(std::vector<Token> tokens) {
    std::map<std::string, std::vector<Token>> pre_variables;

    size_t index = 0;
    while (index < tokens.size() && tokens[index].type != TOKEN_EOF) {
        if (index + 2 < tokens.size() && tokens[index].value == "_" &&
            tokens[index + 1].type == TOKEN_LIT &&
            tokens[index + 2].value == "=") {
            std::string var_name = tokens[index + 1].value;
            size_t start = index + 3;
            size_t index2 = 0;
            std::vector<Token> value;
            while (start + index2 < tokens.size() &&
                   tokens[start + index2].value != "\n" &&
                   tokens[start + index2].type != TOKEN_EOF) {
                value.push_back(tokens[start + index2]);
                index2++;
            }
            index = start + index2;
            value.push_back(
                Token{.value = "\0", .type = TOKEN_EOF, .line = 0, .col = 0});
            pre_variables[var_name] = value;
        } else {
            index++;
        }
    }

    index = 0;
    bool restart = false;
    while (index < tokens.size() && tokens[index].type != TOKEN_EOF) {
        if (index + 2 < tokens.size() && tokens[index].value == "_" &&
            tokens[index + 1].type == TOKEN_LIT &&
            tokens[index + 2].value == "=") {
            std::string var_name = tokens[index + 1].value;
            std::vector<Token> value = pre_variables[tokens[index].value];
            std::vector<Token> new_value;
            size_t index2 = 0;
            while (index2 < value.size() && value[index2].type != TOKEN_EOF) {
                if (index2 + 3 < value.size() && value[index2].value == "$" &&
                    value[index2 + 1].value == "(" &&
                    value[index2 + 2].type == TOKEN_LIT &&
                    value[index2 + 3].value == ")") {
                    std::string ref_var = value[index2 + 2].value;

                    if (value[index2 + 2].type != TOKEN_LIT ||
                        !pre_variables.count(ref_var) || var_name == ref_var) {
                        std::cout << "variable reference error at line: "
                                  << value[index2 + 2].line
                                  << ", column: " << value[index2 + 2].col
                                  << std::endl;

                        exit(1);
                    }
                    for (const Token &token : pre_variables[ref_var]) {
                        if (token.type != TOKEN_EOF) {
                            new_value.push_back(token);
                        }
                    }
                    index2 += 4;
                    restart = true;
                } else {
                    new_value.push_back(tokens[index2]);
                    index2++;
                }
            }
            pre_variables[var_name] = new_value;
        }

        if (restart) {
            index = 0;
            restart = false;
        } else {
            index++;
        }
    }

    index = 0;
    while (index < tokens.size() && tokens[index].type != TOKEN_EOF) {
        if (index + 2 < tokens.size() && tokens[index].value == "_" &&
            tokens[index + 1].type == TOKEN_LIT &&
            tokens[index + 2].value == "=") {
            std::string var_name = tokens[index + 1].value;
            std::string new_value = "";
            for (const Token &token : pre_variables[var_name]) {
                if (token.type != TOKEN_EOF) {
                    new_value += token.value;
                }
            }
            variables[var_name] = new_value;
            index += 3;
        } else {
            index++;
        }
    }
}

void initialize_argument_calls(std::vector<Token> tokens) {
    size_t index = 0;
    while (tokens[index].type != TOKEN_EOF) {
        if (index + 2 < tokens.size() && tokens[index].value == "~" &&
            tokens[index+1].type == TOKEN_LIT && tokens[index+2].value == ":" &&
            tokens[index+3].type == TOKEN_LIT && functions.count(tokens[index+3].value)) {
            string argument = tokens[index+1].value;
            string function = tokens[index+3].value;
            argument_calls[argument] = function;
        }
    }
}
void initialize_shell_commands() {
    
}

void interperet(std::vector<Token> tokens) {
    initialize_variables(tokens);
    initialize_functions(tokens);

    size_t index = 0;
    while (tokens[index].type != TOKEN_EOF) {
        index++;
    }
}
