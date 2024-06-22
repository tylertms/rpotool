#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "convert.h"

int convert_file(char *rpopath, char *objpath)
{
    FILE *rpofile = fopen(rpopath, "rb");
    if (rpofile == NULL)
    {
        printf("File not found: %s\n", rpopath);
        return 1;
    }

    fseek(rpofile, 0, SEEK_END);
    int filelen = ftell(rpofile);
    rewind(rpofile);

    char *buffer = (char *)malloc(filelen * sizeof(char));
    if (buffer == NULL)
    {
        fclose(rpofile);
        return 1;
    }

    fread(buffer, filelen, 1, rpofile);
    fclose(rpofile);

    char *obj_data = convert(buffer, filelen);

    free(buffer);

    if (obj_data == NULL)
    {
        printf("Failed to convert %s\n", rpopath);
        return 1;
    }

    FILE *objfile = fopen(objpath, "w");
    if (objfile == NULL)
    {
        printf("Couldn't create a target .obj file at %s\n", objpath);
        free(obj_data);
        return 1;
    }

    fwrite(obj_data, 1, strlen(obj_data), objfile);
    fclose(objfile);
    free(obj_data);

    printf("Converted %s to %s\n", rpopath, objpath);

    return 0;
}

char *convert(const char *buffer, size_t buffer_size)
{
    char *data = NULL;
    int magic_number;

    if (buffer_size < sizeof(int))
    {
        fprintf(stderr, "Invalid buffer size\n");
        return NULL;
    }

    memcpy(&magic_number, buffer, sizeof(int));

    if (magic_number != RPO1_MAGIC)
    {
        unsigned char *decompressed_data = NULL;
        size_t decompressed_data_len = 0;

        if (zlib_inflate((unsigned char *)buffer, buffer_size, &decompressed_data, &decompressed_data_len) != 0)
        {
            printf("Invalid .rpo(z) file\n");
            return NULL;
        }

        data = (char *)decompressed_data;
        buffer_size = decompressed_data_len;
    }
    else
    {
        data = (char *)buffer;
    }

    int vertices = *(int *)(data + VERTEX_COUNT_INDEX);
    int faces = *(int *)(data + FACE_COUNT_INDEX) / 3;

    size_t obj_buffer_size = vertices * 8 * 8 + faces * 8 * 4;

    char *obj_buffer = (char *)malloc(obj_buffer_size);
    if (obj_buffer == NULL)
    {
        if (data != buffer)
        {
            free(data);
        }
        fprintf(stderr, "Failed to allocate memory for obj_buffer\n");
        return NULL;
    }

    size_t offset = 0;

    for (int i = 0; i < vertices; i++)
    {
        int base = HEADER_SIZE + VERTEX_SIZE * i;
        offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "v ");

        for (int j = 0; j < 6; j++)
        {
            float k = *(float *)(data + base);
            base += 4;
            offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "%f ", k);
        }

        offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "\n");
    }

    offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "\n");

    for (int i = 0; i < faces; i++)
    {
        int base = HEADER_SIZE + VERTEX_SIZE * vertices + FACE_SIZE * i;
        offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "f ");

        for (int j = 0; j < 3; j++)
        {
            int k = *(unsigned short *)(data + base) + 1;
            base += 2;
            offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "%d ", k);
        }

        offset += snprintf(obj_buffer + offset, obj_buffer_size - offset, "\n");
    }

    if (data != buffer)
    {
        free(data);
    }

    return obj_buffer;
}

int zlib_inflate(const unsigned char *source, size_t source_len, unsigned char **dest, size_t *dest_len)
{
    size_t buffer_size = source_len * 2;
    *dest = (unsigned char *)malloc(buffer_size);
    if (*dest == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for decompression buffer\n");
        return -1;
    }

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = (Bytef *)source;
    strm.avail_in = source_len;
    strm.next_out = *dest;
    strm.avail_out = buffer_size;

    if (inflateInit(&strm) != Z_OK)
    {
        fprintf(stderr, "Inflate init failed\n");
        free(*dest);
        return -1;
    }

    int ret;
    while ((ret = inflate(&strm, Z_NO_FLUSH)) != Z_STREAM_END)
    {
        if (ret != Z_OK)
        {
            fprintf(stderr, "Decompress failed with error code %d\n", ret);
            inflateEnd(&strm);
            free(*dest);
            return -1;
        }

        if (strm.avail_out == 0)
        {
            buffer_size *= 2;
            *dest = (unsigned char *)realloc(*dest, buffer_size);
            if (*dest == NULL)
            {
                fprintf(stderr, "Failed to reallocate memory for decompression buffer\n");
                inflateEnd(&strm);
                return -1;
            }

            strm.next_out = *dest + strm.total_out;
            strm.avail_out = buffer_size - strm.total_out;
        }
    }

    *dest_len = strm.total_out;
    inflateEnd(&strm);
    return 0;
}
