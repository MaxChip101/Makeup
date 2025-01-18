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

// Constants
const unsigned int TOKEN_AMOUNT = 5000;

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
    char* value;
    int line;
} Token;

void free_tokens(Token* tokens)
{
    for(long unsigned int i = 0; i < TOKEN_AMOUNT; i++)
    {
        free(tokens[i].value);
    }
    free(tokens);
}

Token* tokenize(char* content)
{
    Token* tokens = malloc(sizeof(Token) * TOKEN_AMOUNT);
    unsigned int token_count = 0;
    unsigned int line = 1;
    unsigned int i = 0;
    char str[2] = "\0";

    while (content[i] != '\0')
    {
        if(content[i] == '#')
        {
            while(content[i] != '\n' || content[i] != '\0')
            {
                i++;
                line++;
            }
            continue;
        }

        // skipping whitespaces
        while (content[i] == ' ' || content[i] == '\t' || content[i] == '\r')
        {
            i++;
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
            tokens[token_count].value = strdup(value);
            token_count++;
            continue;

        }

        switch (content[i])
        {
            case '\n':
                i++;
                line++;
                continue;
            case '(':
                tokens[token_count].type = TOKEN_RPAREN;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case ')':
                tokens[token_count].type = TOKEN_LPAREN;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '{':
                tokens[token_count].type = TOKEN_LBRACE;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '}':
                tokens[token_count].type = TOKEN_RBRACE;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '=':
                tokens[token_count].type = TOKEN_EQUALS;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '$':
                tokens[token_count].type = TOKEN_DOLLAR;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '&':
                tokens[token_count].type = TOKEN_AND;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case '@':
                tokens[token_count].type = TOKEN_AT;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            case ',':
                tokens[token_count].type = TOKEN_COMMA;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
                continue;
            default:
                tokens[token_count].type = TOKEN_STR;
                tokens[token_count].line = line;
                str[0] = content[i];
                tokens[token_count].value = strdup(str);
                token_count++;
                i++;
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
    Token* logic_tokens = malloc(sizeof(Token) * TOKEN_AMOUNT);
    unsigned int logic_i;
    unsigned int i = 0;

    while (tokens[i].type != TOKEN_EOF)
    {
        // variable definition
        if(tokens[i].type == TOKEN_STR && tokens[i+1].type == TOKEN_EQUALS)
        {
            logic_tokens[i].type = TOKEN_VAR;
            logic_tokens[i].value = strdup(tokens[i].value);
            logic_tokens[i].line = tokens[i].line;
            i++;
            continue;
        }
        // funciton reference
        else if(tokens[i].type == TOKEN_AT && tokens[i+1].type == TOKEN_LPAREN && tokens[i+2].type == TOKEN_STR && tokens[i].type == TOKEN_RPAREN)
        {
            logic_tokens[i+2].type = TOKEN_FUNCREF;
            logic_tokens[i+2].value = strdup(tokens[i+2].value);
            logic_tokens[i+2].line = tokens[i+2].line;
            i+=4;
        }
        // variable reference
        else if(tokens[i].type == TOKEN_DOLLAR && tokens[i+1].type == TOKEN_LPAREN && tokens[i+2].type == TOKEN_STR && tokens[i+3].type == TOKEN_RPAREN)
        {
            logic_tokens[logic_i].type = TOKEN_VARREF;
            logic_tokens[logic_i].value = strdup(tokens[i+2].value);
            logic_tokens[logic_i].line = tokens[i+2].line;
            i+=4;
            logic_i++;
        }
        // pass tokens
        else
        {
            logic_tokens[logic_i].type = tokens[i].type;
            logic_tokens[logic_i].value = strdup(tokens[i].value);
            logic_tokens[logic_i].line = tokens[i].line;
            i++;
            logic_i++;
            continue;
        }
    }
    free_tokens(tokens);
    return(logic_tokens);
}

int parse(Token* tokens)
{
    for(unsigned int i = 0; i < TOKEN_AMOUNT; i++)
    {
        printf("('%s', %i) ", tokens[i].value, tokens[i].line);
    }
    free_tokens(tokens);
    return 0;
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
    snprintf(full_cwd, sizeof(full_cwd), "%s%s", cwd, "/Makeup2");
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
            parse(logicize_tokens(tokenize(content)));
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