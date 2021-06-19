#ifndef U4FILE_H
#define U4FILE_H

#include "miniz.h"

/**
 * Represents zip files that game resources can be loaded from.
 */
typedef struct U4ZipPackage {
	int loaded;
	char name[256];
	char path[256];
} U4ZipPackage;

/**
 * An abstract interface for file access.
 */
typedef struct U4FILE {
	FILE *file;
	mz_zip_archive zip_archive;
	mz_zip_archive_file_stat za_stat;
	int za_index;
	uint8_t *fptr;
	long cur;
} U4FILE;

U4FILE *u4fopen_zip(const char *fname, U4ZipPackage *package);

/////////////////////////////////
bool u4isUpgradeAvailable();
bool u4isUpgradeInstalled();

U4FILE *u4fopen(const char *fname);
U4FILE *u4fopen_stdio(const char *fname);

void u4fclose(U4FILE *f);
int u4fseek(U4FILE *f, long offset, int whence);
long u4ftell(U4FILE *f);
size_t u4fread(U4FILE *f, void *ptr, size_t size, size_t nmemb);
int u4fgetc(U4FILE *f);
int u4fgetshort(U4FILE *f);
int u4fputc(int c, U4FILE *f);
long u4flength(U4FILE *f);
void u4find_path(char *path, size_t psize, const char *fname, const char *subpath);
void u4find_conf(char *path, size_t psize, const char *fname);
void u4find_graphics(char *path, size_t psize, const char *fname);

void zu4_read_strtable(U4FILE *f, long offset, char **array, int nstrings);

void zu4_file_stdio_close(U4FILE *u4f);
void zu4_file_zip_close(U4FILE *u4f);

int zu4_file_stdio_seek(U4FILE *u4f, long offset, int whence);
int zu4_file_zip_seek(U4FILE *u4f, long offset, int whence);

long zu4_file_stdio_tell(U4FILE *u4f);
long zu4_file_zip_tell(U4FILE *u4f);

size_t zu4_file_stdio_read(U4FILE*, void*, size_t, size_t);
size_t zu4_file_zip_read(U4FILE*, void*, size_t, size_t);

long zu4_file_stdio_length(U4FILE *u4f);
long zu4_file_zip_length(U4FILE *u4f);

int zu4_file_stdio_getc(U4FILE *u4f);
int zu4_file_zip_getc(U4FILE *u4f);

int zu4_file_stdio_putc(U4FILE *u4f, int c);
int zu4_file_zip_putc(U4FILE *u4f, int c);

int zu4_file_getshort(U4FILE *u4f);

#endif
