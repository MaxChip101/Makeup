#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <unistd.h>
#include <vector>

using namespace std;

// Definitions
const char* MAKEUP_VERSION = "0.0.1";

// Flags
const char* MAKEUP_VERSION_FLAG_SHORT = "-v";
const char* MAKEUP_VERSION_FLAG = "--version";
const char* MAKEUP_COMMANDS_FLAG_SHORT = "-h";
const char* MAKEUP_COMMANDS_FLAG = "--help";
const char* MAKEUP_CWD_FLAG_SHORT = "-d";
const char* MAKEUP_CWD_FLAG = "--directory";

typedef enum
{
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_STR,
    TOKEN_VAR,
    TOKEN_VARREF,
    TOKEN_FUNC,
    TOKEN_FUNCREF,
    TOKEN_EQUALS,
    TOKEN_AT,
    TOKEN_AND,
    TOKEN_DOLLAR,
    TOKEN_COMMA,
    TOKEN_CMD_END,
    TOKEN_EOF,
    TOKEN_UNKOWN
} TokenType;

typedef struct
{
    TokenType type;
    string value;
    int line;
} Token;


vector<Token> tokenize(string content)
{
    vector<Token> tokens;
    Token _token;
    unsigned int line = 1;
    unsigned int i = 0;

    while (content[i] != '\0')
    {
        // skipping whitespaces
        while (content[i] == ' ' || content[i] == '\t' || content[i] == '\r')
        {
            i++;
        }

        if(isalpha(content[i]) || content[i] == '_')
        {
            string value = "";
            while (isalnum(content[i]) || content[i] == '_')
            {
                value += content[i];
                i++;
            }
            _token.value = value;
            _token.type = TOKEN_STR;
            _token.line = line;
            tokens.push_back(_token);
            continue;
        }

        switch (content[i])
        {
            case '\n':
                i++;
                line++;
                continue;
            case '(':
                _token.value = "(";
                _token.type = TOKEN_LPAREN;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case ')':
                _token.value = ")";
                _token.type = TOKEN_RPAREN;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '{':
                _token.value = "{";
                _token.type = TOKEN_LBRACE;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '}':
                _token.value = "}";
                _token.type = TOKEN_RBRACE;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '=':
                _token.value = "=";
                _token.type = TOKEN_EQUALS;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '$':
                _token.value = "$";
                _token.type = TOKEN_DOLLAR;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '&':
                _token.value = "&";
                _token.type = TOKEN_AND;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case '@':
                _token.value = "@";
                _token.type = TOKEN_AT;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            case ',':
                _token.value = ",";
                _token.type = TOKEN_COMMA;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
            default:
                _token.value = string(1, content[i]);
                _token.type = TOKEN_STR;
                _token.line = line;
                tokens.push_back(_token);
                i++;
                continue;
        }
    }

    _token.value = "\0";
    _token.type = TOKEN_EOF;
    _token.line = line;
    tokens.push_back(_token);
    
    return(tokens);
}

vector<Token> logicize_tokens(vector<Token> tokens)
{

    vector<Token> logic_tokens;
    unsigned int i = 0;
    
    Token _token;

    while (tokens[i].type != TOKEN_EOF)
    {
        // variable definition
        if(tokens[i].type == TOKEN_STR && tokens[i+1].type == TOKEN_EQUALS)
        {
            _token.value = tokens[i].value;
            _token.type = TOKEN_VAR;
            _token.line = tokens[i].line;
            logic_tokens.push_back(_token);
            i++;
            continue;
        }
        // funciton reference
        else if(tokens[i].type == TOKEN_AT && tokens[i+1].type == TOKEN_LPAREN && tokens[i+2].type == TOKEN_STR && tokens[i+3].type == TOKEN_RPAREN)
        {
            _token.value = tokens[i+2].value;
            _token.type = TOKEN_FUNCREF;
            _token.line = tokens[i+2].line;
            logic_tokens.push_back(_token);
            i+=4;
        }
        // variable reference
        else if(tokens[i].type == TOKEN_DOLLAR && tokens[i+1].type == TOKEN_LPAREN && tokens[i+2].type == TOKEN_STR && tokens[i+3].type == TOKEN_RPAREN)
        {
            _token.value = tokens[i+2].value;
            _token.type = TOKEN_VARREF;
            _token.line = tokens[i+2].line;
            logic_tokens.push_back(_token);
            i+=4;
            
        }
        // pass tokens
        else
        {
            _token.value = tokens[i].value;
            _token.type = tokens[i].type;
            _token.line = tokens[i].line;
            logic_tokens.push_back(_token);
            i++;
            continue;
        }
    }

    return(logic_tokens);
}

int parse(vector<Token> tokens)
{
    for(size_t i = 0; i < tokens.size(); i++)
    {
        cout << '(' << tokens[i].value << ", " << tokens[i].line << ')' << endl;
    }
    return 0;
}


int main(int argc, char* argv[])
{

    char cwd[1024];
    string tempcwd = getcwd(cwd, sizeof(cwd));
    
    string full_cwd = tempcwd + "/Makeup";

    // check for flags
    if(argc > 1)
    {
        if(strcmp(argv[1], MAKEUP_COMMANDS_FLAG_SHORT) == 0 || strcmp(argv[1], MAKEUP_COMMANDS_FLAG) == 0)
        {
            cout << "Options:\n -h, --help\tPrints list of commands.\n -v, --version\tPrints Makeup version.\n -d, --cwd\tPrints the current working directory." << endl;
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_VERSION_FLAG) == 0 || strcmp(argv[1], MAKEUP_VERSION_FLAG_SHORT) == 0)
        {
            cout << "Makeup version " << MAKEUP_VERSION << endl;
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_CWD_FLAG) == 0 || strcmp(argv[1], MAKEUP_CWD_FLAG_SHORT) == 0)
        {
            cout << filesystem::current_path() << endl;
            exit(EXIT_SUCCESS);
        }
    }

    // check for Makeup file
    ifstream makeup_file(full_cwd.c_str());
    if(!makeup_file.is_open()) {
        cout << "Makeup file does not exist in '" << filesystem::current_path() << "', please make a Makeup file.\ntype `makeup -h` or `makeup --help` for flags" << endl;
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
    parse(logicize_tokens(tokenize(content)));
    return(0);
}