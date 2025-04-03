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

#include <memory>

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
    TOKEN_SYMBOL,
    TOKEN_LIT,
    TOKEN_NUM,
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
std::map<int, std::string> shell_commands;

std::vector<Token> tokenize(std::string content);
std::string exec(const char *cmd);
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
        if (content[index] == '#') {
            commented = !commented;
            if (!commented) {
                index++;
                col++;
            }
            continue;
        } else if (content[index] == '\n') {
            commented = false;
            value = '\n';
            type = TOKEN_SYMBOL;
            line++;
            index++;
            col = 0;
        } else if (isalpha(content[index])) {
            value.clear();
            while (index < content.length() && (isalnum(content[index]) || content[index] == '_')) {
                value.push_back(content[index]);
                index++;
                col++;
            }
            type = TOKEN_LIT;
        } else if (isdigit(content[index])) {
            value.clear();
            while (index < content.length() && isdigit(content[index])) {
                value.push_back(content[index]);
                index++;
                col++;
            }
            type = TOKEN_NUM;
        } else {
            if ((content[index] == ' ' || content[index] == '\t') && !include_spaces) {
                index++;
                col++;
                continue;
            } else if (index + 1 < content.length() && content[index] == '(' && content[index+1] == '\"' && !include_spaces) {
                index += 2;
                col++;
                include_spaces = true;
                continue;
            } else if (index + 1 < content.length() && content[index] == '\"' && content[index+1] == ')' && include_spaces) {
                index += 2;
                col++;
                include_spaces = false;
                continue;
            } else {
                type = TOKEN_SYMBOL;
                value = content[index];
                col++;
                index++;
            }
        }

        if (commented) {
            index++;
            col++;
            continue;
        } else {
            tokens.push_back(
                Token{.value = value, .type = type, .line = line, .col = col});
        }
    }
    
    tokens.push_back(
        Token{.value = "\0", .type = TOKEN_EOF, .line = line, .col = col});

    return (tokens);
}

std::string exec(const char *cmd) {
    std::string result;
    char buffer[128];

    FILE *pipe = popen(cmd, "r");
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
    std::vector<std::string> stack = {""};
    size_t stack_num = 0;
    size_t index = 0;;

    while (index < input.length() && input[index] != '\0') {
        if(index + 1 < input.length() && input[index] == '!' && input[index+1] == '(') {
            index +=2;
            stack_num++;
            if(stack.size() <= stack_num) {
                stack.push_back("");
            } else {
                stack[stack_num].clear();
            }
            continue;
        } else if(input[index] == ')') {
            std::string command = exec(stack[stack_num].c_str());
            stack[stack_num - 1].append(command);
            stack_num--;
            index++;
            continue;
        }
        stack[stack_num].push_back(input[index]);
        index++;
    }
    return stack[0];
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
        if (index + 2 < tokens.size() && tokens[index].value == ":" &&
            tokens[index + 1].type == TOKEN_LIT &&
            tokens[index + 2].value == "=") {
            std::string var_name = tokens[index+1].value;
            std::vector<Token> value;
            index += 3;
            while (tokens[index].value != "\n") {
                value.push_back(tokens[index]);
                index++;
            }
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
        if (index + 2 < tokens.size() && tokens[index].value == ":" &&
            tokens[index + 1].type == TOKEN_LIT &&
            tokens[index + 2].value == "=") {
            std::string var_name = tokens[index+1].value;
            index += 3;
            size_t index2 = 0;
            std::vector<Token> new_value;
            while (index2 < pre_variables[var_name].size() && pre_variables[var_name][index2].value != "\n") {
                if(index2 + 3 < pre_variables[var_name].size() && pre_variables[var_name][index2].value == "$" &&
                    pre_variables[var_name][index2 + 1].value == "(" &&
                    pre_variables[var_name][index2 + 2].type == TOKEN_LIT &&
                    pre_variables[var_name][index2 + 3].value == ")") {
                    restart = true;
                    std::string var_reference = pre_variables[var_name][index2 + 2].value;
                    new_value.insert(std::end(new_value), std::begin(pre_variables[var_reference]), std::end(pre_variables[var_reference]));
                    index2+=4;
                } else {
                    new_value.push_back(pre_variables[var_name][index2]);
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
        if (index + 2 < tokens.size() && tokens[index].value == ":" &&
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
        if (index + 3 < tokens.size() && tokens[index].value == "~" &&
            tokens[index+1].type == TOKEN_LIT && tokens[index+2].value == ":" &&
            tokens[index+3].type == TOKEN_LIT && functions.count(tokens[index+3].value)) {
            std::string argument = tokens[index+1].value;
            std::string function = tokens[index+3].value;
            argument_calls[argument] = function;
            index += 4;
        }
        else
        {
            index++;
        }
    }
}
void initialize_shell_commands(std::vector<Token> tokens) {
    size_t index = 0;
    while (tokens[index].type != TOKEN_EOF) {
        if (index + 1 < tokens.size() && tokens[index].value == "!" &&
            tokens[index+1].value == "(") {
            std::string value = "";
            while (index < tokens.size() && tokens[index].value != "\n") {
                value.append(tokens[index].value);
                index++;
            }
            // put string value into shell map and get the variable references
        }
        else
        {
            index++;
        }
    }
}

void interperet(std::vector<Token> tokens) {
    initialize_variables(tokens);
    initialize_functions(tokens);
    initialize_argument_calls(tokens);

    size_t index = 0;
    while (tokens[index].type != TOKEN_EOF) {
        //std::cout << tokens[index].value << "\n" ;
        index++;
    }
}
