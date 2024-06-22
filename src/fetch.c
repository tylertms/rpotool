#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <curl/curl.h>

#include "convert.h"
#include "fetch.h"

#define CONFIG_URL "https://gist.githubusercontent.com/tylertms/7592bcbdd1b6891bdf9b2d1a4216b55b/raw/"
#define SHELLS_URL "https://auxbrain.com/dlc/shells/"

set sets[MAX_SETS];
set decorators[MAX_SETS];
shell shells[MAX_SHELLS];
chicken chickens[MAX_SHELLS];
asset_type shell_types[MAX_TYPES];
asset_type chicken_types[MAX_TYPES];

int set_count = 0;
int decorator_count = 0;
int shell_count = 0;
int chicken_count = 0;
int shell_type_count = 0;
int chicken_type_count = 0;

int search_depth = 0;
char *search_results[1000];

struct curl_mem *fetch(const char *url) {
    CURL *curl_handle;
    struct curl_mem *chunk = malloc(sizeof(struct curl_mem));
    if (!chunk) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }

    chunk->memory = malloc(1);
    if (!chunk->memory) {
        fprintf(stderr, "malloc failed\n");
        free(chunk);
        return NULL;
    }

    chunk->size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    CURLcode res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk->memory);
        free(chunk);
        chunk = NULL;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk;
}


size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct curl_mem *mem = (struct curl_mem *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        fprintf(stderr, "realloc() failed\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void download_and_convert(char *filepath)
{
    size_t url_len = strlen(SHELLS_URL) + strlen(filepath);
    char *url = (char *)malloc(url_len + 1);
    if (url == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for URL\n");
        return;
    }
    strcpy(url, SHELLS_URL);
    strcat(url, filepath);

    struct curl_mem *rpoz = fetch(url);
    if (rpoz == NULL)
    {
        fprintf(stderr, "Failed to fetch data from URL: %s\n", url);
        free(url);
        return;
    }

    char *obj_data = convert(rpoz->memory, rpoz->size);
    if (obj_data == NULL)
    {
        fprintf(stderr, "Failed to convert data from URL: %s\n", url);
        free(url);
        free(rpoz->memory);
        free(rpoz);
        return;
    }

    char *objpath = (char *)malloc(strlen(filepath) + 10); 
    if (objpath == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for objpath\n");
        free(url);
        free(obj_data);
        free(rpoz->memory);
        free(rpoz);
        return;
    }
    strcpy(objpath, "output/");
    strcat(objpath, filepath);

    char *dot = strrchr(objpath, '.');
    if (dot != NULL)
    {
        *dot = '\0';
    }
    strcat(objpath, ".obj");

    FILE *file = fopen(objpath, "w");
    if (file)
    {
        fwrite(obj_data, 1, strlen(obj_data), file);
        fclose(file);
        printf("Successfully created %s\n", objpath);
    }
    else
    {
        fprintf(stderr, "Failed to write to file: %s\n", objpath);
    }

    free(obj_data);
    free(objpath);
    free(url);

    free(rpoz->memory);
    free(rpoz);
}


void download_search_result(char *download_selection)
{
    int index = atoi(download_selection);

    if (make_directory("output") != 0)
    {
        printf("Failed to create output directory.\n");
        return;
    }

    if (strcmp(download_selection, "a") == 0)
    {
        for (int i = 0; search_results[i] != NULL; i++)
        {
            download_and_convert(search_results[i]);
        }
    }
    else if (index > 0 && search_results[index - 1] != NULL)
    {
        download_and_convert(search_results[index - 1]);
    }
    search_depth = 0;
}

int make_directory(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0700) != 0)
        {
            fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
            return -1;
        }
    }

    return 0;
}

void print_menu()
{
    printf("\n\nSearch by:\n");
    printf("1 - Set\n");
    printf("2 - Decorator\n");
    printf("3 - Building\n");
    printf("4 - Chicken\n");
    printf("q - Quit\n\n");
    printf("Enter your choice: ");
}

void print_broad_selection_type(char *broad)
{
    int index = atoi(broad);
    char *type = "";
    switch (index)
    {
    case 1:
        type = "set number";
        break;
    case 2:
        type = "decorator number";
        break;
    case 3:
        type = "building type";
        break;
    case 4:
        type = "chicken type";
        break;
    }

    printf("\n\nOptions:\n");
    printf("# - Filter shells by the corresponding %s.\n", type);
    printf("0 - Back.\n");
    printf("q - Quit.\n\n");
    printf("Enter your choice: ");
}

void print_specific_selection_type()
{
    printf("\n\nOptions:\n");
    printf("# - Download a specific shell by its number.\n");
    printf("a - Download all shells in the list.\n");
    printf("0 - Back.\n");
    printf("q - Quit.\n\n");
    printf("Enter your choice: ");
}

void print_broad_filter_list(char *broad)
{
    int index = atoi(broad);

    switch (index)
    {
    case 1:
        for (int i = 0; i < set_count; i++)
        {
            printf("\n%d - %s", i + 1, sets[i].name);
        }
        break;

    case 2:
        for (int i = 0; i < decorator_count; i++)
        {
            printf("\n%d - %s", i + 1, decorators[i].name);
        }
        break;

    case 3:
        for (int i = 0; i < shell_type_count; i++)
        {
            printf("\n%d - %s", i + 1, shell_types[i].type);
        }
        break;

    case 4:
        for (int i = 0; i < chicken_type_count; i++)
        {
            printf("\n%d - %s", i + 1, chicken_types[i].type);
        }
        break;

    default:
        search_depth--;
        break;
    }
}

void print_specific_filter_list(char *broad, char *specific)
{
    int broad_index = atoi(broad);
    int specific_index = atoi(specific);
    int index = 0;

    switch (broad_index)
    {
    case 1:
        if (specific_index < 1 || specific_index > set_count)
        {
            search_depth--;
            break;
        }

        for (int i = 0; i < shell_count; i++)
        {
            if (strcmp(sets[specific_index - 1].name, shells[i].set_name) == 0)
            {
                search_results[index++] = shells[i].url_key;
                search_results[index] = NULL;
                printf("\n%d - %s (%s)", index, shells[i].shell_type, sets[specific_index - 1].name);
            }
        }
        break;

    case 2:
        if (specific_index < 1 || specific_index > decorator_count)
        {
            search_depth--;
            break;
        }

        for (int i = 0; i < shell_count; i++)
        {
            if (strcmp(decorators[specific_index - 1].name, shells[i].set_name) == 0)
            {
                search_results[index++] = shells[i].url_key;
                search_results[index] = NULL;
                printf("\n%d - %s (%s)", index, shells[i].shell_type, decorators[specific_index - 1].name);
            }
        }
        break;

    case 3:
        if (specific_index < 1 || specific_index > shell_type_count)
        {
            search_depth--;
            break;
        }

        for (int i = 0; i < shell_count; i++)
        {
            if (strcmp(shell_types[specific_index - 1].type, shells[i].shell_type) == 0)
            {
                search_results[index++] = shells[i].url_key;
                search_results[index] = NULL;
                printf("\n%d - %s (%s)", index, shells[i].shell_type, shells[i].set_name);
            }
        }
        break;

    case 4:
        if (specific_index < 1 || specific_index > chicken_type_count)
        {
            search_depth--;
            break;
        }

        for (int i = 0; i < chicken_count; i++)
        {
            if (strcmp(chicken_types[specific_index - 1].type, chickens[i].chicken_name) == 0)
            {
                search_results[index++] = chickens[i].url_key;
                search_results[index] = NULL;
                printf("\n%d - %s (%s)", index, chickens[i].chicken_name, chickens[i].identifier);
            }
        }
        break;

    default:
        search_depth--;
        break;
    }
}

void add_to_category(char *category, char *tokens[])
{
    if (strcmp(category, "set") == 0)
    {
        sets[set_count].identifier = strdup(tokens[0]);
        sets[set_count++].name = strdup(tokens[1]);
    }
    else if (strcmp(category, "decorator") == 0)
    {
        decorators[decorator_count].identifier = strdup(tokens[0]);
        decorators[decorator_count++].name = strdup(tokens[1]);
    }
    else if (strcmp(category, "shell") == 0)
    {
        shells[shell_count].identifier = strdup(tokens[0]);
        shells[shell_count].set_name = strdup(tokens[1]);
        shells[shell_count].shell_type = strdup(tokens[2]);
        shells[shell_count++].url_key = strdup(tokens[3]);
    }
    else if (strcmp(category, "chicken") == 0)
    {
        chickens[chicken_count].identifier = strdup(tokens[0]);
        chickens[chicken_count].chicken_name = strdup(tokens[1]);
        chickens[chicken_count].shell_type = strdup(tokens[2]);
        chickens[chicken_count++].url_key = strdup(tokens[3]);
    }
    else if (strcmp(category, "shell_type") == 0)
    {
        shell_types[shell_type_count++].type = strdup(tokens[0]);
    }
    else if (strcmp(category, "chicken_type") == 0)
    {
        chicken_types[chicken_type_count++].type = strdup(tokens[0]);
    }
}

void parse_csv(const char *data)
{
    printf("Parsing shell data...   ");

    char *config = strdup(data);
    if (config == NULL)
    {
        perror("strdup");
        return;
    }

    char *end_str;
    char *line = strtok_r(config, "\n", &end_str);

    while (line != NULL)
    {
        char *tokens[6];
        char *end_token;
        int token_count = 0;

        char *token = strtok_r(line, ",", &end_token);
        while (token != NULL && token_count < 6)
        {
            tokens[token_count++] = token;
            token = strtok_r(NULL, ",", &end_token);
        }

        add_to_category(tokens[0], tokens + 1);

        line = strtok_r(NULL, "\n", &end_str);
    }

    free(config);

    printf("Done!\n");
    printf("Found %d sets, %d decorators, %d shells (%d types), and %d chickens (%d types).\n", set_count, decorator_count, shell_count, shell_type_count, chicken_count, chicken_type_count);
}

void begin_cli()
{
    clear();

    struct curl_mem *chunk = fetch(CONFIG_URL);
    if (chunk && chunk->memory)
    {
        printf("Fetching shell data... Done!\n");
        parse_csv(chunk->memory);
        free(chunk->memory);
        free(chunk);
    }
    else
    {
        printf("Failed to fetch data.\n");
    }

    char broad_choice[10] = "";
    char specific_choice[10] = "";
    char download_selection[10] = "";

    while (1)
    {
        char *choice;

        switch (search_depth)
        {
        case 0:
            choice = broad_choice;
            print_menu();
            break;

        case 1:
            choice = specific_choice;
            print_broad_selection_type(broad_choice);
            break;

        case 2:
            choice = download_selection;
            print_specific_selection_type();
            break;
        }

        scanf(" %9s", choice);
        clear();

        if (strcmp(choice, "q") == 0)
        {
            break;
        }

        else if (strcmp(choice, "0") == 0)
        {
            if (search_depth > 0)
            {
                search_depth--;
                if (search_depth == 1)
                {
                    strcpy(specific_choice, "");
                }
                else if (search_depth == 0)
                {
                    strcpy(broad_choice, "");
                }
            }
        }
        else
        {
            search_depth++;
        }

        if (search_depth == 1)
        {
            print_broad_filter_list(broad_choice);
        }
        else if (search_depth == 2)
        {
            print_specific_filter_list(broad_choice, specific_choice);
        }
        else
        {
            download_search_result(download_selection);
        }
    }

    printf("Exiting...\n");
}