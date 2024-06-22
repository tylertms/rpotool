#ifndef FETCH_H
#define FETCH_H

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <direct.h> 
#define mkdir(path, mode) _mkdir(path)
#define clear() system("cls")
#else
#include <curl/curl.h>
#define clear() system("tput reset")
#endif

#define MAX_SETS 100
#define MAX_SHELLS 10000
#define MAX_TYPES 1000

typedef struct {
    char *identifier;
    char *name;
} set;

typedef struct {
    char *identifier;
    char *set_name;
    char *shell_type;
    char *url_key;
} shell;

typedef struct {
    char *identifier;
    char *chicken_name;
    char *shell_type;
    char *url_key;
} chicken;

typedef struct {
    char *type;
} asset_type;


struct curl_mem {
    char *memory;
    size_t size;
};

struct curl_mem *fetch(const char *url);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
int make_directory(const char *path);
void print_menu();
void print_broad_selection_type(char *broad);
void print_specific_selection_type();
void print_broad_filter_list(char *broad);
void print_specific_filter_list(char *broad, char *specific);
void download_and_convert(char *filepath);
void download_search_result(char *download_selection);
void add_to_category(char *category, char *tokens[]);
void parse_csv(const char *data);
void begin_cli();

#endif // FETCH_H
