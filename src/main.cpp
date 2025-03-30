#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <unistd.h>
#include <bits/stdc++.h>
#include <cstdio>
#include <memory>
#include <vector>

using namespace std;

// Definitions
const char* MAKEUP_VERSION = "0.0.1";

// Flags
const char* MAKEUP_VERSION_FLAG_SHORT = "-v";
const char* MAKEUP_VERSION_FLAG = "--version";
const char* MAKEUP_COMMANDS_FLAG_SHORT = "-h";
const char* MAKEUP_COMMANDS_FLAG = "--help";
const char* MAKEUP_DIRECTORY_FLAG_SHORT = "-d";
const char* MAKEUP_DIRECTORY_FLAG = "--directory";

typedef enum
{
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

typedef struct
{
    string value;
    TokenType type;
    int line;
    int col;
} Token;

map<string, string> variables;
map<string, int> functions;


vector<Token> tokenize(string content);
string exec(const string &cmd);
string shell_command(string input);
void initialize_functions(vector<Token> tokens);
void initialize_variables(vector<Token> tokens);
void initialize_interperet(vector<Token> tokens);


int main(int argc, char* argv[])
{
    printf("%s", exec("ls").c_str());
    char cwd[1024];
    string tempcwd = getcwd(cwd, sizeof(cwd));
    string full_cwd = tempcwd + "/Makeup";

    // check for flags
    if(argc > 1)
    {
        if(strcmp(argv[1], MAKEUP_COMMANDS_FLAG_SHORT) == 0 || strcmp(argv[1], MAKEUP_COMMANDS_FLAG) == 0)
        {
            cout << "Options:\n " << MAKEUP_COMMANDS_FLAG_SHORT << ", " << MAKEUP_COMMANDS_FLAG << "\tPrints list of commands.\n " << MAKEUP_VERSION_FLAG_SHORT << ", " << MAKEUP_VERSION_FLAG << "\tPrints Makeup version.\n " << MAKEUP_DIRECTORY_FLAG_SHORT << ", " << MAKEUP_DIRECTORY_FLAG << "\tPrints the current working directory." << endl;
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_VERSION_FLAG) == 0 || strcmp(argv[1], MAKEUP_VERSION_FLAG_SHORT) == 0)
        {
            cout << "Makeup version " << MAKEUP_VERSION << endl;
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_DIRECTORY_FLAG) == 0 || strcmp(argv[1], MAKEUP_DIRECTORY_FLAG_SHORT) == 0)
        {
            cout << filesystem::current_path() << endl;
            exit(EXIT_SUCCESS);
        }
    }

    // check for Makeup file
    ifstream makeup_file(full_cwd.c_str());
    if(!makeup_file.is_open()) {
        cerr << "Makeup file does not exist in " << filesystem::current_path() << ", please make a Makeup file.\ntype `makeup -h` or `makeup --help` for flags" << endl;
        exit(EXIT_FAILURE);
    }
    string content;

    string buffer;
    while(getline(makeup_file, buffer))
    {
        content += buffer;
        content.push_back('\n');
    }
    makeup_file.close();

    return(0);
}

vector<Token> tokenize(string content)
{
    int index = 0;
    int line = 1;
    int col = 1;
    bool include_spaces = false;
    bool commented = false;
    TokenType type;
    string value = "";
    vector<Token> tokens;
    while(content[index] != '\0')
    {
        if(isalpha(content[index]))
        {
            type = TOKEN_LIT;
            int start = index;
            int index2 = 0;
            while (isalnum(content[start + index2]) || content[start + index2] == '_')
            {
                value.push_back(content[start + index2]);
                index2++;
            }
            index += index2;
        }
        else
        {
            if(content[index] == '(' && content[index+1] == '\"' && !include_spaces)
            {
                include_spaces = true;
                index+=2;
                col += 2;
            }
            else if(content[index] == '\"' && content[index+1] == ')' && !include_spaces)
            {
                include_spaces = false;
                index+=2;
                col+=2;
                continue;
            }
            else if(content[index] == '\n') { // make it so that double quotes preserve white spaces and work on shell commands in variables and stuff and make the tokens become strings in variable declarations
                line++;
                col = 0;
                commented = false;
            }
            else if(content[index] == ' ' || content[index] == '\t' && !include_spaces)
            {
                col++;
                continue;
            }
            else if(content[index] == '#')
            {
                commented = !commented;
            }
            value = content[index];
            type = TOKEN_SYMBOL;
        }
        if(!commented)
        {
            tokens.push_back(Token{.value = "", .type = type, .line = line, .col = col});
        }
        col++;
        index++;
    }

    tokens.push_back(Token{.value = "\0", .type = TOKEN_EOF, .line = line, .col = col});

    return(tokens);
}

string exec(const string &cmd)
{
    string result;
    char buffer[128];

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return "\0";
    }

    while(!feof(pipe))
    {
        if(fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            result += buffer;
        }
    }

    pclose(pipe);
    return result;
}

string shell_command(string input)
{
    string final_string = "";
    string reversed_string = "";
    for(int index = input.length()-1; index >= 0;  index--)
    {
        if(input[index] == '!' && input[index+1] == '(')
        {
            index += 2;
            int index2 = index;
            string command = "";
            while(input[index2] != ')')
            {
                command.push_back(input[index2]);
                index2++;
            }
            string value = exec(command);
            for(int index3 = value.length() - 1; index3 >= 0; index3--)
            {
                reversed_string += value[index3];
            }
        }
        else if(input[index] == ')')
        {
            continue;
        }
        else
        {
            reversed_string += input[index];
        }
    }

    for(int index2 = reversed_string.length() - 1; index2 >= 0; index2--)
    {
        final_string += reversed_string[index2];
    }

    return final_string;
}

void initialize_functions(vector<Token> tokens)
{

}



void initialize_variables(vector<Token> tokens)
{
    map<string, vector<Token>> pre_variables;
    
    int index = 0;
    while(tokens[index].type != TOKEN_EOF)
    {
        if(tokens[index].value.compare("_") && tokens[index+1].type == TOKEN_LIT && tokens[index+2].value.compare("="))
        {
            int start = index;
            int index2 = 0;
            vector<Token> value;
            while(!tokens[start + index2].value.compare("\n") && tokens[start + index2].type != TOKEN_EOF)
            {
                value.push_back(tokens[start + index2]);
                index2++;
            }
            index += index2;
            value.push_back(Token{.value = "\0", .type = TOKEN_EOF, .line = 0, .col = 0});
            pre_variables.insert({tokens[index].value, value});
        }
    }

    index = 0;
    bool restart = false;
    while(tokens[index].type != TOKEN_EOF)
    {
        if(tokens[index].value.compare("_") && tokens[index+1].type == TOKEN_LIT && tokens[index+2].value.compare("="))
        {
            vector<Token> value = pre_variables[tokens[index].value];
            vector<Token> new_value;
            int index2 = 0;
            while (value[index2].type != TOKEN_EOF)
            {
                if(value[index2].value.compare("$") && value[index2+1].value.compare("(") && value[index2+2].type == TOKEN_LIT && value[index2+3].value.compare(")"))
                {
                    restart = true;
                    if(value[index2+2].type != TOKEN_LIT || !pre_variables.count(value[index2+2].value) || tokens[index].value.compare(value[index2+2].value))
                    {
                        printf("variable reference error at line=%i, column=%i", value[index2+2].line, value[index2+2].col);
                        exit(1);
                    }
                    for(Token token : pre_variables[value[index2+2].value])
                    {
                        new_value.push_back(token);
                    }
                    index2 += 3;
                }
                else
                {
                    new_value.push_back(tokens[index2]);
                }
                index2++;
            }
            pre_variables[value[index2+2].value] = new_value;
        }

        if(restart)
        {
            index = 0;
            restart = false;
        }
    }

    index = 0;
    while(tokens[index].type != TOKEN_EOF)
    {
        if(tokens[index].value.compare("_") && tokens[index+1].type == TOKEN_LIT && tokens[index+2].value.compare("="))
        {
            vector<Token> value = pre_variables[tokens[index].value];
            string new_value = "";
            int index2 = 0;
            while (value[index2].type != TOKEN_EOF)
            {
                new_value += value[index2].value;
                index2++;
            }
            variables.insert({tokens[index+1].value, new_value});
        }
    }
}

void initialize_interperet(vector<Token> tokens)
{
    initialize_variables(tokens);

    int index = 0;
    while(tokens[index].type != TOKEN_EOF)
    {
        index++;
    }
}