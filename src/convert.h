#ifndef CONVERT_H
#define CONVERT_H

#define RPO1_MAGIC 0x314F5052
#define HEADER_SIZE 0x40
#define VERTEX_COUNT_INDEX 0x04
#define FACE_COUNT_INDEX 0x3C
#define VERTEX_SIZE 0x28
#define FACE_SIZE 0x06

int convert_file(char *rpopath, char *objpath);
char* convert(const char *buffer, size_t buffer_size);
int zlib_inflate(const unsigned char *source, size_t source_len, unsigned char **dest, size_t *dest_len);

#endif // CONVERT_H
