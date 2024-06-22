#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "convert.h"
#include "fetch.h"

void print_help(char *exec);
void print_convert_help(char *exec);
void print_fetch_help(char *exec);

int main(int argc, char **argv)
{
    char *objpath = NULL;

    if (argc == 1)
    {
        print_help(argv[0]);
        return 1;
    }

    int help = 0;
    char *filepath = NULL;
    char *output = NULL;
    for (int arg = 2; arg < argc; arg++)
    {
        if (argv[arg][0] != '-')
        {
            if (filepath != NULL)
            {
                free(filepath);
            }
            filepath = (char *)malloc(strlen(argv[arg]) + 1);
            strcpy(filepath, argv[arg]);
            continue;
        }

        switch (argv[arg][1])
        {
        case 'o':
            if (arg + 1 >= argc)
            {
                print_convert_help(argv[0]);
                return 1;
            }

            output = argv[++arg];
            break;

        case 'h':
            help = 1;
            break;

        case '-':
            if (strcmp(argv[arg], "--output") == 0)
            {
                output = argv[++arg];
            }
            else if (strcmp(argv[arg], "--help") == 0)
            {
                help = 1;
            }
            break;

        default:
            print_help(argv[0]);
        }
    }

    if (strcmp(argv[1], "fetch") == 0)
    {
        if (help)
        {
            print_fetch_help(argv[0]);
            return 0;
        }

        begin_cli();
        return 0;
    }
    else if (strcmp(argv[1], "convert") == 0)
    {
        if (help || filepath == NULL)
        {
            print_convert_help(argv[0]);
            return 0;
        }

        if (output == NULL)
        {
            size_t len = strlen(filepath);
            output = (char *)malloc(len + 5);
            strcpy(output, filepath);

            char *dot = strrchr(output, '.');
            if (dot != NULL)
            {
                *dot = '\0';
            }
        }

        if (output != NULL && strstr(output, ".obj") == NULL)
        {
            strcat(output, ".obj");
        }

        convert_file(filepath, output);
        return 0;
    }
    else
    {
        print_help(argv[0]);
        return 1;
    }

    return 0;
}

void print_help(char *exec)
{
    printf("Command-line tool to fetch and convert Egg, Inc. .rpo and .rpoz files to .obj files.\n\n");
    printf("Usage:\n");
    printf("  %s [command] [options]\n\n", exec);
    printf("Available Commands:\n");
    printf("  convert    Convert an .rpo(z) file to an .obj file\n");
    printf("  fetch      Browse and convert shells interactively\n\n");
    printf("Flags:\n");
    printf("  -h, --help      Display the help message for a command\n");
    printf("\n");
}

void print_convert_help(char *exec)
{
    printf("\nConvert an Egg, Inc. .rpo(z) file to the .obj format.\n\n");
    printf("Usage:\n");
    printf("  %s convert <file.rpo(z)> [flags]\n\n", exec);
    printf("Flags:\n");
    printf("  -o, --output    Specify the name of the output file\n");
    printf("  -h, --help      Display this help message\n");
    printf("\n");
}

void print_fetch_help(char *exec)
{
    printf("\nInteractively browse and download available .rpoz shells as .obj files.\n\n");
    printf("Usage:\n");
    printf("  %s fetch [flags]\n\n", exec);
    printf("Flags:\n");
    printf("  -h, --help      Display this help message\n");
    printf("\n");
}