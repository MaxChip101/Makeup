#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

/* Definitions */
const char* MAKEUP_VERSION = "0.0.1";

/* Flags */
const char* MAKEUP_CREATE_FLAG = "start";
const char* MAKEUP_BUILD_FLAG = "build";
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

    if(argc >= 1)
    {
        if(argv[1] == MAKEUP_COMMANDS_FLAG_SHORT || argv[1] == MAKEUP_COMMANDS_FLAG)
        {

        }
        else if(argv[1] == MAKEUP_VERSION || argv[1] == MAKEUP_VERSION_FLAG_SHORT)
        {
            printf("Makeup version %s\n", MAKEUP_VERSION);
        }
    }
    // check for Makeup file
    char full_cwd[PATH_MAX];
    snprintf(full_cwd, sizeof(full_cwd), "%s%s", cwd, "/Makeup");
    if(access(full_cwd, F_OK) != 0)
    {
        printf("'Makeup' file does not exist in '%s', please make a Makeup file\n", cwd);
        exit(1);
    }

    return(0);
}