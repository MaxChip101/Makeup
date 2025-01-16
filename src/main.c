#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

/* Definitions */
const char* MAKEUP_VERSION = "0.0.1";

/* Flags */
const char* MAKEUP_CREATE_FLAG = "create";
const char* MAKEUP_VERSION_FLAG_SHORT = "-v";
const char* MAKEUP_VERSION_FLAG = "--version";
const char* MAKEUP_COMMANDS_FLAG_SHORT = "-h";
const char* MAKEUP_COMMANDS_FLAG = "--help";




FILE *file;

int main(int argc, char* argv[])
{
    // get cwd
    char buf[PATH_MAX];
    char* cwd = getcwd(buf, sizeof(buf));
    if(cwd == NULL)
    {
        puts("Could not get CWD\n");
        exit(1);
    }

    // check for flags
    if(argc > 1)
    {
        if(strcmp(argv[1], MAKEUP_COMMANDS_FLAG_SHORT) == 0 || strcmp(argv[1], MAKEUP_COMMANDS_FLAG) == 0)
        {
            exit(0);
        }
        else if(strcmp(argv[1], MAKEUP_VERSION_FLAG) == 0 || strcmp(argv[1], MAKEUP_VERSION_FLAG_SHORT) == 0)
        {
            printf("Makeup version %s\n", MAKEUP_VERSION);
            exit(0);
        }
        else if(strcmp(argv[1], MAKEUP_CREATE_FLAG) == 0 || strcmp(argv[1], MAKEUP_VERSION_FLAG_SHORT) == 0)
        {
            printf("Created template Makeup file at '%s'\n", cwd);
            exit(0);
        }
    }
    // check for Makeup file
    char full_cwd[PATH_MAX];
    snprintf(full_cwd, sizeof(full_cwd), "%s%s", cwd, "/Makeup");
    if(access(full_cwd, F_OK) != 0)
    {
        printf("Makeup file does not exist in '%s', please make a Makeup file\n", cwd);
        exit(1);
    }

    return(0);
}