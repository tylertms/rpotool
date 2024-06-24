#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>
#include <ctype.h>

#include "naett.h"

#define CHUNK 16384
#define RPO1_MAGIC 0x314F5052
#define RPOZ_MAGIC 0x9C78
#define CHUNK_INDICATOR 0x1406
#define CONFIG_URL "https://gist.github.com/tylertms/197ea8c6f9d63b027def62f0091782c4/raw/"
#define DLC_URL "https://auxbrain.com/dlc/shells/"

#ifdef _WIN32
#define mkdir(path, mode) mkdir(path)
#endif

int process_path(char *path, char *outputPath);
int read_file(char *filename, unsigned char **data, size_t *len);
int process_rpo_file(char *filepath, char *outputPath);
int convert_rpo_to_obj(unsigned char *rpoData, size_t rpoLen, char *objPath);

int request(char *URL, char *method, unsigned char **data, size_t *len);
int decompress(unsigned char *source, size_t sourceLen, unsigned char **dest, size_t *destLen);
int search_config(char *searchValue, char *outputPath);

int determine_file_type(char *filename);
void print_with_padding(char *str, int len, char delim);
void print_bytes_with_padding(long bytes);
void add_url(char ***array, int *size, int *capacity, const char *str);


int main(int argc, char **argv)
{

    char *inputPath = NULL;
    char *outputPath = NULL;
    char *searchValue = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search") == 0)
            searchValue = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
            outputPath = argv[++i];
        else
            inputPath = argv[i];
    }

    if (searchValue != NULL)
    {
        return search_config(searchValue, outputPath);
    }
    else if (inputPath != NULL)
    {
        return process_path(inputPath, outputPath);
    }

    printf("Usage: %s <folder or file.rpo(z)> [options]\n", argv[0]);
    printf("Options:\n");
    printf("  -s, --search <term>       Search for shells and download as .obj \n");
    printf("  -o, --output <path>       Output file or folder for the converted file(s)\n");

    return 0;
}

int process_path(char *path, char *outputPath)
{
    int fileType = determine_file_type(path);
    if (fileType == -1)
    {
        fprintf(stderr, "error: invalid file or directory: %s\n", path);
        return 1;
    }

    if (fileType == 0)
    {
        char *ext = strrchr(path, '.');
        if (!ext || (strcmp(ext, ".rpo") != 0 && strcmp(ext, ".rpoz") != 0))
        {
            fprintf(stderr, "error: file %s does not have a .rpo or .rpoz extension\n", path);
            return 1;
        }

        if (determine_file_type(outputPath) == 1)
        {
            fprintf(stderr, "error: %s is a directory\n", outputPath);
            return 1;
        }

        if (outputPath && strcmp(outputPath + strlen(outputPath) - 4, ".obj") != 0)
        {
            char *newOutputPath = (char *)malloc(strlen(outputPath) + 5);
            if (!newOutputPath)
            {
                fprintf(stderr, "error: failed to allocate memory for output path\n");
                return 1;
            }
            strcpy(newOutputPath, outputPath);
            strcat(newOutputPath, ".obj");

            int status = process_rpo_file(path, newOutputPath);
            free(newOutputPath);
            return status;
        }

        return process_rpo_file(path, outputPath);
    }

    DIR *dir = opendir(path);
    if (!dir)
    {
        fprintf(stderr, "error: failed to open directory: %s\n", path);
        return 1;
    }

    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL)
    {
        char *filename = dirent->d_name;
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
            continue;

        char *ext = strrchr(filename, '.');
        if (!ext || (strcmp(ext, ".rpo") != 0 && strcmp(ext, ".rpoz") != 0))
            continue;

        char *rpoPath = (char *)malloc(strlen(path) + strlen(filename) + 2);
        if (!rpoPath)
        {
            fprintf(stderr, "error: failed to allocate memory for rpo path\n");
            closedir(dir);
            return 1;
        }

        if (outputPath == NULL)
        {
            outputPath = path;
        }

        if (determine_file_type(outputPath) == -1)
        {
            if (mkdir(outputPath, 0777) == -1)
            {
                fprintf(stderr, "error: failed to create directory %s\n", outputPath);
                free(rpoPath);
                closedir(dir);
                return 1;
            }
        }
        else if (determine_file_type(outputPath) == 0)
        {
            fprintf(stderr, "error: %s is not a directory\n", outputPath);
            free(rpoPath);
            closedir(dir);
            return 1;
        }

        strcpy(rpoPath, path);
        if (path[strlen(path) - 1] != '/')
            strcat(rpoPath, "/");
        strcat(rpoPath, filename);

        char *outputOBJpath = (char *)malloc(strlen(outputPath) + strlen(filename) + 2);
        if (!outputOBJpath)
        {
            fprintf(stderr, "error: failed to allocate memory for output obj path\n");
            free(rpoPath);
            closedir(dir);
            return 1;
        }

        strcpy(outputOBJpath, outputPath);
        if (outputPath[strlen(outputPath) - 1] != '/')
            strcat(outputOBJpath, "/");
        strtok(filename, ".");
        strcat(outputOBJpath, filename);
        strcat(outputOBJpath, ".obj");

        if (process_rpo_file(rpoPath, outputOBJpath) != 0)
        {
            free(rpoPath);

            continue;
        }

        free(rpoPath);
    }
    closedir(dir);

    return 0;
}

int read_file(char *filename, unsigned char **data, size_t *len)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "error: failed to open file: %s\n", filename);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    *len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *data = (unsigned char *)malloc(*len);
    if (!*data || fread(*data, 1, *len, fp) != *len)
    {
        fprintf(stderr, "error: failed to allocate memory or read file: %s\n", filename);
        fclose(fp);
        free(*data);
        return 1;
    }

    fclose(fp);
    return 0;
}

int process_rpo_file(char *filepath, char *outputPath)
{
    size_t rpoLen = 0;
    unsigned char *rpo = NULL;

    if (read_file(filepath, &rpo, &rpoLen) != 0)
    {
        free(rpo);
        return 1;
    }

    char *objPath = NULL;
    if (outputPath)
    {
        objPath = outputPath;
    }
    else
    {
        objPath = (char *)malloc(strlen(filepath) + 5);
        if (objPath == NULL)
        {
            fprintf(stderr, "error: failed to allocate memory for obj path\n");
            free(rpo);
            return 1;
        }
        strcpy(objPath, filepath);
        char *ext = strrchr(objPath, '.');
        if (ext)
            *ext = '\0';
        strcat(objPath, ".obj");
    }

    if (convert_rpo_to_obj(rpo, rpoLen, objPath) != 0)
    {
        fprintf(stderr, "error: failed to convert file: %s\n", filepath);
        free(objPath);
        free(rpo);
        return 1;
    }

    free(objPath);
    free(rpo);
    return 0;
}

int convert_rpo_to_obj(unsigned char *rpoData, size_t rpoLen, char *objPath)
{
    int fromCompressed = 0;

    if (*(unsigned short *)rpoData == RPOZ_MAGIC)
    {
        fromCompressed = 1;

        size_t lenOut = 0;
        unsigned char *rpoOut = NULL;
        int result = decompress(rpoData, rpoLen, &rpoOut, &lenOut);
        if (result != Z_OK)
        {
            fprintf(stderr, "error: decompression failed with error code: %d\n", result);
            free(rpoOut);
            return 1;
        }
        rpoData = rpoOut;
        rpoLen = lenOut;
    }

    if (*(unsigned int *)rpoData != RPO1_MAGIC)
    {
        fprintf(stderr, "error: invalid .rpo file: %s\n", objPath);
        return 1;
    }

    FILE *objFile = fopen(objPath, "wb");
    if (!objFile)
    {
        fprintf(stderr, "error: failed to open obj file: %s\n", objPath);
        return 1;
    }

    char *rpoPath = (char *)malloc(strlen(objPath) + 5 + fromCompressed);
    if (rpoPath == NULL)
    {
        fprintf(stderr, "error: failed to allocate memory for rpo path\n");
        fclose(objFile);
        return 1;
    }
    strcpy(rpoPath, strrchr(objPath, '/') + 1);
    strtok(rpoPath, ".");
    strcat(rpoPath, ".rpo");
    if (fromCompressed)
        strcat(rpoPath, "z");

    fprintf(objFile, "# Converted from %s\n", rpoPath);
    fprintf(objFile, "# This file is property of Auxbrain, Inc. and should be treated as such.\n\n");

    free(rpoPath);

    int vertices = *(int *)(rpoData + 0x4);
    int faces = *(int *)(rpoData + 0x8) / 6;
    int headerLen = 0, vertexLen = 0;

    for (int addr = 0; addr < rpoLen; addr += 4)
    {
        unsigned int token = *(unsigned int *)(rpoData + addr);
        if (token == CHUNK_INDICATOR)
        {
            vertexLen += 4 * *(unsigned int *)(rpoData + addr - 4);
        }
        if (token == 0 && *(unsigned int *)(rpoData + addr + 4) > 4)
        {
            headerLen = addr + 8;
            break;
        }
    }

    for (int i = 0; i < vertices; i++)
    {
        int tokens = vertexLen / 4 - (vertexLen / 4) % 3;
        if (tokens > 6)
            tokens = 6;

        fprintf(objFile, "v ");
        for (int j = 0; j < tokens; j++)
        {
            float k = *(float *)(rpoData + headerLen + i * vertexLen + j * 4);
            fprintf(objFile, "%f ", k);
        }
        fprintf(objFile, "\n");
    }

    fprintf(objFile, "\n");

    for (int i = 0; i < faces; i++)
    {
        fprintf(objFile, "f ");
        for (int j = 0; j < 3; j++)
        {
            unsigned short k = *(unsigned short *)(rpoData + headerLen + vertices * vertexLen + i * 6 + j * 2) + 1;
            fprintf(objFile, "%d ", k);
        }
        fprintf(objFile, "\n");
    }

    fclose(objFile);
    strtok(objPath, ".");
    printf("info: created %s.obj from .rpo(z)\n", strchr(objPath, '/') + 1);

    return 0;
}

int request(char *URL, char *method, unsigned char **data, size_t *len)
{
    naettReq *req = naettRequest(URL, naettMethod(method), naettHeader("accept", "*/*"));
    naettRes *res = naettMake(req);

    while (!naettComplete(res))
    {
        usleep(100 * 1000);
    }

    int status = naettGetStatus(res);

    if (status < 0)
    {
        return 1;
    }

    naettGetBody(res, (int *)len);

    *data = (unsigned char *)malloc(*len + 1);
    if (!*data)
    {
        naettClose(res);
        naettFree(req);
        return 1;
    }

    memcpy(*data, naettGetBody(res, (int *)len), *len);

    naettClose(res);
    naettFree(req);

    return 0;
}

int decompress(unsigned char *source, size_t sourceLen, unsigned char **dest, size_t *destLen)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    size_t sourceOffset = 0, destOffset = 0, destCapacity = CHUNK;

    *dest = (unsigned char *)malloc(destCapacity);
    if (!*dest)
        return Z_MEM_ERROR;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
        free(*dest);
        return ret;
    }

    do
    {
        strm.avail_in = sourceLen - sourceOffset < CHUNK ? sourceLen - sourceOffset : CHUNK;
        memcpy(in, source + sourceOffset, strm.avail_in);
        sourceOffset += strm.avail_in;
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);

            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
            {
                inflateEnd(&strm);
                free(*dest);
                return ret;
            }
            have = CHUNK - strm.avail_out;

            if (destOffset + have > destCapacity)
            {
                destCapacity = destCapacity * 2 > destOffset + have ? destCapacity * 2 : destOffset + have;
                unsigned char *newDest = (unsigned char *)realloc(*dest, destCapacity);
                if (!newDest)
                {
                    inflateEnd(&strm);
                    free(*dest);
                    return Z_MEM_ERROR;
                }
                *dest = newDest;
            }

            memcpy(*dest + destOffset, out, have);
            destOffset += have;
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    *destLen = destOffset;
    if (ret == Z_STREAM_END)
    {
        unsigned char *finalDest = (unsigned char *)realloc(*dest, destOffset);
        if (finalDest)
            *dest = finalDest;
        return Z_OK;
    }
    else
    {
        free(*dest);
        return Z_DATA_ERROR;
    }
}

int search_config(char *searchValue, char *outputPath)
{
    naettInit(NULL);

    size_t configLen = 0;
    unsigned char *config = NULL;
    if (request(CONFIG_URL, "GET", &config, &configLen) != 0)
    {
        fprintf(stderr, "error: failed to request config\n");
        return 1;
    }

    if (config == NULL)
    {
        fprintf(stderr, "error: failed to get config\n");
        return 1;
    }

    for (int i = 0; i < strlen(searchValue); i++)
    {
        searchValue[i] = tolower(searchValue[i]);
    }

    int category = 0;
    int shellsize = 0, shellcount = 0;
    int chickensize = 0, chickencount = 0;

    char *line = strtok((char *)config, "\n");

    int urlcap = 16;
    int size = 0;
    char **urlarr = malloc(urlcap * sizeof(char *));

    if (urlarr == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        free(config);
        return 1;
    }

    while (line)
    {
        char *lineLower = (char *)malloc(strlen(line) + 1);
        if (!lineLower)
        {
            fprintf(stderr, "error: failed to allocate memory for line lower\n");
            free(config);
            free(urlarr);
            return 1;
        }
        for (int i = 0; i < strlen(line); i++)
        {
            lineLower[i] = tolower(line[i]);
        }
        lineLower[strlen(line)] = '\0';

        if (strstr(lineLower, searchValue))
        {
            if (category < 1 && lineLower[0] == 's')
            {
                printf("\nShell");
                for (int i = 0; i < 34; i++)
                    printf(" ");
                printf("|       Size | ID\n");
                for (int i = 0; i < 100; i++)
                    printf("-");
                printf("\n");
                category = 1;
            }
            else if (category < 2 && lineLower[0] == 'c')
            {
                printf("\nChicken");
                for (int i = 0; i < 34; i++)
                    printf(" ");
                printf("|       Size | ID\n");
                for (int i = 0; i < 100; i++)
                    printf("-");
                printf("\n");
                category = 2;
            }

            char *sizestr = strrchr(line, '|') + 1;
            int lineSize = atoi(sizestr);

            if (lineSize == 0)
            {
                fprintf(stderr, "error: invalid size for %s\n", line);
                free(lineLower);
                break;
            }

            switch (category)
            {
            case 1:
                shellcount++;
                shellsize += lineSize;
                break;
            case 2:
                chickencount++;
                chickensize += lineSize;
                break;
            }

            print_with_padding(strchr(line, '|') + 1, 40, '|');
            printf(" | ");
            print_bytes_with_padding(lineSize);
            printf(" | ");

            strrchr(line, '|')[0] = '\0';
            line = strchr(strchr(line, '|') + 1, '|') + 1;

            char *id = strrchr(line, '|') + 1;
            char *url = (char *)malloc(strlen(DLC_URL) + strlen(line) + 6);
            if (!url)
            {
                fprintf(stderr, "error: failed to allocate memory for url\n");
                free(lineLower);
                break;
            }
            strcpy(url, DLC_URL);
            strrchr(line, '|')[0] = '\0';

            strcat(url, line);
            strcat(url, "_");
            strcat(url, id);
            strcat(url, ".rpoz");

            add_url(&urlarr, &size, &urlcap, url);
            free(url); // Free the URL memory after adding to the array

            printf("%s\n", line);
        }
        line = strtok(NULL, "\n");
        free(lineLower);
    }

    free(config);

    if (shellcount == 0 && chickencount == 0)
    {
        printf("No results found for \"%s\"\n", searchValue);
        free(urlarr);
        return 0;
    }

    printf("\n\nSummary");
    for (int i = 0; i < 9; i++)
        printf(" ");
    printf("|       Size \n");
    for (int i = 0; i < 29; i++)
        printf("-");
    printf("\n%d shells  ", shellcount);
    if (shellcount == 0)
        shellcount = 1;
    for (int i = 0; i < 5 - floor(log10(abs(shellcount))); i++)
        printf(" ");
    printf(" | ");
    print_bytes_with_padding(shellsize);

    printf("\n%d chickens", chickencount);
    if (chickencount == 0)
        chickencount = 1;
    for (int i = 0; i < 5 - floor(log10(abs(chickencount))); i++)
        printf(" ");
    printf(" | ");
    print_bytes_with_padding(chickensize);

    printf("\n\nTotal:");
    print_bytes_with_padding(shellsize + chickensize);

    if (outputPath == NULL)
        printf("\nDownload all to your current folder? [Y/n] ");
    else
        printf("\nDownload all to \"%s\"? [Y/n] ", outputPath);

    char c = getchar();
    if (c != 'y' && c != 'Y')
    {
        for (int i = 0; i < size; i++)
        {
            free(urlarr[i]);
        }
        free(urlarr);
        return 0;
    }

    if (outputPath != NULL)
    {
        if (determine_file_type(outputPath) != 1)
        {
            if (mkdir(outputPath, 0777) == -1)
            {
                fprintf(stderr, "error: %s must be a valid directory\n", outputPath);
                free(config);
                return 1;
            }
        }
    }

    for (int i = 0; i < size; i++)
    {
        size_t rpoLen = 0;
        unsigned char *rpo = NULL;

        if (request(urlarr[i], "GET", &rpo, &rpoLen) != 0)
        {
            fprintf(stderr, "error: failed to request %s\n", urlarr[i]);
            for (int i = 0; i < size; i++)
            {
                free(urlarr[i]);
            }
            free(urlarr);
            return 1;
        }

        char *objPath = (char *)malloc(strlen(outputPath) + strlen(urlarr[i]) - strlen(DLC_URL) + 5);
        if (!objPath)
        {
            fprintf(stderr, "error: failed to allocate memory for obj path\n");
            free(rpo);
            for (int i = 0; i < size; i++)
            {
                free(urlarr[i]);
            }
            free(urlarr);
            return 1;
        }
        strcpy(objPath, outputPath);
        if (outputPath[strlen(outputPath) - 1] != '/')
            strcat(objPath, "/");
        strcat(objPath, urlarr[i] + strlen(DLC_URL));
        strtok(objPath, ".");
        strcat(objPath, ".obj");

        if (convert_rpo_to_obj(rpo, rpoLen, objPath) != 0)
        {
            fprintf(stderr, "error: failed to convert file: %s\n", urlarr[i]);
            free(rpo);
            free(objPath);
            for (int i = 0; i < size; i++)
            {
                free(urlarr[i]);
            }
            free(urlarr);
            return 1;
        }

        free(rpo);
        free(objPath);
        free(urlarr[i]);
    }
    free(urlarr);
    return 0;
}

int determine_file_type(char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        if (st.st_mode & S_IFDIR)
            return 1;
        if (st.st_mode & S_IFREG)
            return 0;
    }
    return -1;
}

void print_with_padding(char *str, int len, char delim)
{
    int sel = 1;
    int strLen = strlen(str);

    for (int i = 0; i < len; i++)
    {
        if (i >= strLen || str[i] == delim)
            sel = 0;

        if (sel)
            printf("%c", str[i]);
        else
            printf(" ");
    }
}

void print_bytes_with_padding(long bytes)
{
    char *suffix[] = {" B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double dblBytes = bytes;

    if (bytes > 1024)
    {
        for (i = 0; (bytes / 1024) > 0 && i < 4; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }

    printf("%7.2f %s", dblBytes, suffix[i]);
}

void add_url(char ***array, int *size, int *capacity, const char *str)
{
    if (*size >= *capacity)
    {
        *capacity *= 2;
        *array = realloc(*array, *capacity * sizeof(char *));
        if (*array == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }
    (*array)[*size] = malloc(strlen(str) + 1);
    if ((*array)[*size] == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    strcpy((*array)[*size], str);
    (*size)++;
}
