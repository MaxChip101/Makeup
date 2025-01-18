#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

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
    TOKEN_LN,
    TOKEN_EOF,
    TOKEN_UNKOWN
} TokenType;

typedef struct
{
    TokenType type;
    char* value;
    int line;
} Token;

Token* tokenize(char* content)
{
    Token* tokens = malloc(sizeof(Token) * 1000);
    int token_count = 0;
    char token_name;
    int line = 1;
    int i = 0;
    while (content[i] != '\0')
    {
        // skipping whitespaces
        while (content[i] == ' ' || content[i] == '\t' || content[i] == '\r')
        {
            i++;
        }

        switch (content[i])
        {
            case '\n':
                tokens[token_count].type = TOKEN_LN;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                line++;
                continue;
            case '(':
                tokens[token_count].type = TOKEN_RPAREN;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case ')':
                tokens[token_count].type = TOKEN_LPAREN;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '{':
                tokens[token_count].type = TOKEN_LBRACE;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '}':
                tokens[token_count].type = TOKEN_RBRACE;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '=':
                tokens[token_count].type = TOKEN_EQUALS;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '$':
                tokens[token_count].type = TOKEN_DOLLAR;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '&':
                tokens[token_count].type = TOKEN_AND;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case '@':
                tokens[token_count].type = TOKEN_AT;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
            case ',':
                tokens[token_count].type = TOKEN_COMMA;
                tokens[token_count].line = line;
                tokens[token_count].value = strdup(content[i]);
                token_count++;
                i++;
                continue;
        }

        if(isalpha(content[i]) || content[i] == '_')
        {
            int start = i;
            while (isalnum(content[i]) || content[i] == '_')
            {
                i++;
            }
            
            int length = i - start;
            char* value = malloc(length + 1);
            strncpy(value, &content[start], length);
            value[length] = '\0';

            tokens[token_count].type = TOKEN_STR;
            tokens[token_count].line = line;
            tokens[token_count].value = strdup(content[i]);
            token_count++;
            continue;

        }
    }

    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = strdup("EOF");
    tokens[token_count].line = line;
    token_count++;
    
    return tokens;
}

Token* logicize_tokens(Token* tokens)
{
    Token* logic_tokens = malloc(sizeof(tokens));
    int i = 0;

    while (tokens[i].type != TOKEN_EOF)
    {
        if(tokens[i].type == TOKEN_STR && tokens[i+1].type == TOKEN_EQUALS)
        {
            logic_tokens[i].type = TOKEN_VAR;
            logic_tokens[i].value = tokens[i].value;
            logic_tokens[i].line = tokens[i].line;
            i++;
            continue; // make it remove the equals
        }
        else if(tokens[i].type == TOKEN_AT && tokens[i+1].type == TOKEN_LPAREN && tokens[i+2].type == TOKEN_STR && tokens[i].type == TOKEN_RPAREN)
        {
            // make entire thing make a function def
        }
        else
        {
            logic_tokens[i] = tokens[i];
            i++;
            continue;
        }
    }
    
}

int parse(Token* tokens)
{

}


int main(int argc, char* argv[])
{
    // get cwd
    char buf[PATH_MAX];
    char* cwd = getcwd(buf, sizeof(buf));
    if(cwd == NULL)
    {
        puts("Could not get CWD\n");
        exit(EXIT_FAILURE);
    }

    // check for flags
    if(argc > 1)
    {
        if(strcmp(argv[1], MAKEUP_COMMANDS_FLAG_SHORT) == 0 || strcmp(argv[1], MAKEUP_COMMANDS_FLAG) == 0)
        {
            puts("Options:\n -h, --help\tPrints list of commands.\n -v, --version\tPrints Makeup version.\n -d, --cwd\tPrints the current working directory.\n");
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_VERSION_FLAG) == 0 || strcmp(argv[1], MAKEUP_VERSION_FLAG_SHORT) == 0)
        {
            printf("Makeup version %s\n", MAKEUP_VERSION);
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[1], MAKEUP_CWD_FLAG) == 0 || strcmp(argv[1], MAKEUP_CWD_FLAG_SHORT) == 0)
        {
            printf("%s\n", cwd);
            exit(EXIT_SUCCESS);
        }
    }

    // check for Makeup file
    char full_cwd[PATH_MAX];
    snprintf(full_cwd, sizeof(full_cwd), "%s%s", cwd, "/Makeup");
    if(access(full_cwd, F_OK) == 0)
    {
        FILE* makeup_file = fopen(full_cwd, "r");
        if(makeup_file != NULL)
        {
            fseek(makeup_file, 0, SEEK_END);
            long size = ftell(makeup_file);
            rewind(makeup_file);
            char *content = malloc(size + 1);
            if(content == NULL)
            {
                puts("Memory allocation has failed");
                exit(EXIT_FAILURE);
            }
            size_t read_size = fread(content, 1, size, makeup_file);
            content[read_size] = '\0';
            fclose(makeup_file);
            logicize_tokens(tokenize(content));
            puts(content);
            free(content);
        }
        else
        {
            printf("Failed to read Makeup file '%s'", full_cwd);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("Makeup file does not exist in '%s', please make a Makeup file.\ntype `makeup -h` or `makeup --help` for flags\n", cwd);
        exit(EXIT_FAILURE);
    }

    return(0);
}