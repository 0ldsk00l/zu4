/*
 * u4file.cpp
 * Copyright (C) 2014 xu4 Team
 * Copyright (C) 2020-2021 R. Danbrook
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "u4file.h"

void (*zu4_file_close)(U4FILE*);
int (*zu4_file_seek)(U4FILE*, long offset, int whence);
long (*zu4_file_tell)(U4FILE*);
size_t (*zu4_file_read)(U4FILE*, void *ptr, size_t size, size_t nmemb);
long (*zu4_file_length)(U4FILE*);
int (*zu4_file_getc)(U4FILE*);
int (*zu4_file_putc)(U4FILE*, int);

static struct _u4paths {
	const char *rootpath = "./";
	const char *dospath = "ultima4";
	const char *zippath = "./";
	const char *confpath = "conf";
	const char *graphicspath = "graphics";
} u4paths;

static U4ZipPackage u4base;
static U4ZipPackage u4upgrade;

bool u4isUpgradeAvailable() {
	bool avail = false;
	U4FILE *pal;
	if ((pal = u4fopen("u4vga.pal")) != NULL) {
		avail = true;
		u4fclose(pal);
	}
	return avail;
}

bool u4isUpgradeInstalled() {
	U4FILE *u4f = NULL;
	long int filelength;
	bool result = false;

	/* FIXME: Is there a better way to determine this? */
	u4f = u4fopen("ega.drv");
	if (u4f) {
		filelength = u4flength(u4f);
		u4fclose(u4f);
		/* see if (ega.drv > 5k).  If so, the upgrade is installed */
		if (filelength > (5 * 1024)) { result = true; }
	}

	zu4_error(ZU4_LOG_DBG, "u4isUpgradeInstalled %d\n", (int)result);

	return result;
}

/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * First, it looks in the zipfiles.  Next, it tries FILENAME, Filename
 * and filename in up to four paths, meaning up to twelve or more
 * opens per file.  Seems to be ok for performance, but could be
 * getting excessive.  The presence of the zipfiles should probably be
 * cached.
 */

U4FILE *u4fopen(const char *fname) {
	U4FILE *u4f = NULL;

	zu4_error(ZU4_LOG_DBG, "looking for %s\n", fname);

	if (!u4base.loaded) {
		// Check for the default zip packages
		char path[64];
		u4find_path(path, sizeof(path), "ultima4.zip", u4paths.zippath);

		if (path[0] != '\0') {
			// Open the zip
			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));

			// Check zip file validity
			if (!mz_zip_reader_init_file(&zip_archive, path, 0)) {
				zu4_error(ZU4_LOG_ERR, "Archive corrupt, exiting...\n");
			}

			// Locate file to detect directory structure inside archive
			// Revisit this, because miniz may not care about directory case sensitivity FIXME
			if (mz_zip_reader_locate_file(&zip_archive, "charset.ega", NULL, 0) >= 0) {
				snprintf(u4base.name, sizeof(u4base.name), "%s", path);
				snprintf(u4base.path, sizeof(u4base.path), "%s", "");
				u4base.loaded = 1;
			}

			mz_zip_reader_end(&zip_archive);
		}
	}

	if (!u4upgrade.loaded) {
		char path[64];
		u4find_path(path, sizeof(path), "u4upgrad.zip", u4paths.zippath);

		if (path[0] != '\0') {
			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));

			if (!mz_zip_reader_init_file(&zip_archive, path, 0)) {
				zu4_error(ZU4_LOG_ERR, "Archive corrupt, exiting...\n");
			}

			if (mz_zip_reader_locate_file(&zip_archive, "u4vga.pal", NULL, 0) >= 0) {
				snprintf(u4upgrade.name, sizeof(u4upgrade.name), "%s", path);
				snprintf(u4upgrade.path, sizeof(u4upgrade.path), "%s", "");
				u4upgrade.loaded = 1;
			}

			mz_zip_reader_end(&zip_archive);
		}
	}

	u4f = u4fopen_zip(fname, &u4base);
	if (u4f) {
		zu4_file_close = &zu4_file_zip_close;
		zu4_file_seek = &zu4_file_zip_seek;
		zu4_file_tell = &zu4_file_zip_tell;
		zu4_file_read = &zu4_file_zip_read;
		zu4_file_length = &zu4_file_zip_length;
		zu4_file_getc = &zu4_file_zip_getc;
		zu4_file_putc = &zu4_file_zip_putc;
		return u4f; /* file was found, return it! */
	}
	
	u4f = u4fopen_zip(fname, &u4upgrade);
	if (u4f) {
		zu4_file_close = &zu4_file_zip_close;
		zu4_file_seek = &zu4_file_zip_seek;
		zu4_file_tell = &zu4_file_zip_tell;
		zu4_file_read = &zu4_file_zip_read;
		zu4_file_length = &zu4_file_zip_length;
		zu4_file_getc = &zu4_file_zip_getc;
		zu4_file_putc = &zu4_file_zip_putc;
		return u4f; /* file was found, return it! */
	}

	// file not in a zipfile; check if it has been unzipped
	char fname_copy[64];
	snprintf(fname_copy, sizeof(fname_copy), "%s", fname);

	char path[64];
	u4find_path(path, sizeof(path), fname_copy, u4paths.dospath);

	if (path[0] == '\0') {
		if (islower(fname_copy[0])) {
			fname_copy[0] = toupper(fname_copy[0]);
			u4find_path(path, sizeof(path), fname_copy, u4paths.dospath);
		}

		if (path[0] == '\0') {
			for (unsigned int i = 0; fname_copy[i] != '\0'; i++) {
				if (islower(fname_copy[i]))
					fname_copy[i] = toupper(fname_copy[i]);
			}
			u4find_path(path, sizeof(path), fname_copy, u4paths.dospath);
		}
	}

	if (path[0] != '\0') {
		u4f = u4fopen_stdio(path);
		if (u4f != NULL) {
			zu4_error(ZU4_LOG_DBG, "%s successfully opened\n", path);
		}
	}

	zu4_file_close = &zu4_file_stdio_close;
	zu4_file_seek = &zu4_file_stdio_seek;
	zu4_file_tell = &zu4_file_stdio_tell;
	zu4_file_read = &zu4_file_stdio_read;
	zu4_file_length = &zu4_file_stdio_length;
	zu4_file_getc = &zu4_file_stdio_getc;
	zu4_file_putc = &zu4_file_stdio_putc;
	return u4f;
}

/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE.
 */
U4FILE *u4fopen_stdio(const char *fname) {
	U4FILE *u4f;
	FILE *f;

	f = fopen(fname, "rb");
	if (!f) { return NULL; }

	u4f = new U4FILE;
	u4f->file = f;

	return u4f;
}

/**
 * Opens a file from a zipfile and wraps it in a U4FILE.
 */
U4FILE *u4fopen_zip(const char *fname, U4ZipPackage *package) {
	U4FILE *u4f;

	mz_zip_archive za;
	memset(&za, 0, sizeof(za));

	// Check zip file validity
	if (!mz_zip_reader_init_file(&za, package->name, 0)) {
		return NULL;
	}

	char pathname[320];
	snprintf(pathname, sizeof(pathname), "%s%s", package->path, fname);

	int index = mz_zip_reader_locate_file(&za, pathname, NULL, 0);
	if (index == -1) {
		mz_zip_reader_end(&za);
		return NULL;
	}

	mz_zip_archive_file_stat stat;
	mz_zip_reader_file_stat(&za, index, &stat);
	void *ptr = mz_zip_reader_extract_to_heap(&za, index, NULL, 0);

	u4f = new U4FILE;
	u4f->zip_archive = za;
	u4f->za_index = index;
	u4f->za_stat = stat;
	u4f->fptr = (uint8_t*)ptr;
	u4f->cur = 0;

	return u4f;
}

void u4fclose(U4FILE *f) {
	zu4_file_close(f);
	delete f;
}

int u4fseek(U4FILE *f, long offset, int whence) {
	return zu4_file_seek(f, offset, whence);
}

long u4ftell(U4FILE *f) {
	return zu4_file_tell(f);
}

size_t u4fread(U4FILE *f, void *ptr, size_t size, size_t nmemb) {
	return zu4_file_read(f, ptr, size, nmemb);
}

int u4fgetc(U4FILE *f) {
	return zu4_file_getc(f);
}

int u4fgetshort(U4FILE *f) {
	return zu4_file_getshort(f);
}

int u4fputc(int c, U4FILE *f) {
	return zu4_file_putc(f, c);
}

long u4flength(U4FILE *f) {
	return zu4_file_length(f);
}

/**
 * Read a series of zero terminated strings from a file.  The strings
 * are read from the given offset, or the current file position if
 * offset is -1.
 */
void zu4_read_strtable(U4FILE *f, long offset, char **array, int nstrings) {
	zu4_assert(offset < u4flength(f), "offset begins beyond end of file");

	char buffer[512];

	if (offset != -1) { u4fseek(f, offset, SEEK_SET); }

	for (int i = 0; i < nstrings; i++) {
		int j = 0;
		char c;

		while ((c = zu4_file_getc(f)) != '\0') {
			buffer[j++] = c;
		}

		array[i] = (char*)malloc(sizeof(char) * j);
		strncpy(array[i], buffer, j);
		memset(buffer, 0, sizeof(buffer));
	}
}

void u4find_path(char *path, size_t psize, const char *fname, const char *subpath) {
	FILE *f = NULL;

	f = fopen(fname, "rb");
	if (f) { snprintf(path, psize, "%s", fname); }

	// Try paths
	if (f == NULL) {
		snprintf(path, psize, "%s/%s/%s", u4paths.rootpath, subpath, fname);

		zu4_error(ZU4_LOG_DBG, "trying to open %s\n", path);

		if ((f = fopen(path, "rb")) != NULL) {
			zu4_error(ZU4_LOG_DBG, "u4file opened %s\n", path);
		}
		else {
			memset(path, '\0', psize);
		}
	}

	f != NULL ? zu4_error(ZU4_LOG_DBG, "%s successfully found\n", path) :
	zu4_error(ZU4_LOG_DBG, "%s not found\n", fname);

	if (f) { fclose(f); }
}

void u4find_conf(char *path, size_t psize, const char *fname) {
	u4find_path(path, psize, fname, u4paths.confpath);
}

void u4find_graphics(char *path, size_t psize, const char *fname) {
	u4find_path(path, psize, fname, u4paths.graphicspath);
}

int zu4_file_stdio_seek(U4FILE *u4f, long offset, int whence) {
	return fseek(u4f->file, offset, whence);
}

int zu4_file_zip_seek(U4FILE *u4f, long offset, int whence) {
	long pos;

	zu4_assert(whence != SEEK_END, "seeking with whence == SEEK_END not allowed with zipfiles");
	pos = u4f->cur;

	if (whence == SEEK_CUR) {
		offset = pos + offset;
	}

	if (offset == pos) {
		return 0;
	}

	if (offset < pos) {
		pos = 0;
		u4f->cur = 0;
	}

	zu4_assert(offset - pos > 0, "error in U4FILE_zip::seek");
	u4f->cur += offset - pos;
	return 0;
}

void zu4_file_stdio_close(U4FILE *u4f) {
	fclose(u4f->file);
}

void zu4_file_zip_close(U4FILE *u4f) {
	mz_zip_reader_end(&(u4f->zip_archive));
	free(u4f->fptr);
}

long zu4_file_stdio_tell(U4FILE *u4f) {
	return ftell(u4f->file);
}

long zu4_file_zip_tell(U4FILE *u4f) {
	return u4f->cur;
}

size_t zu4_file_stdio_read(U4FILE *u4f, void *ptr, size_t size, size_t nmemb) {
	return fread(ptr, size, nmemb, u4f->file);
}

size_t zu4_file_zip_read(U4FILE *u4f, void *ptr, size_t size, size_t nmemb) {
	size_t retval = nmemb * size;
	if (retval) {
		memcpy(ptr, u4f->fptr + u4f->cur, retval);
		u4f->cur += retval;
	}
	return retval;
}

long zu4_file_stdio_length(U4FILE *u4f) {
	long curr, len;

	curr = ftell(u4f->file);
	fseek(u4f->file, 0L, SEEK_END);
	len = ftell(u4f->file);
	fseek(u4f->file, curr, SEEK_SET);

	return len;
}

long zu4_file_zip_length(U4FILE *u4f) {
	return u4f->za_stat.m_uncomp_size;
}

int zu4_file_stdio_getc(U4FILE *u4f) {
	return fgetc(u4f->file);
}

int zu4_file_zip_getc(U4FILE *u4f) {
	uint8_t *retptr = u4f->fptr;
	return (int)retptr[u4f->cur++];
}

int zu4_file_stdio_putc(U4FILE *u4f, int c) {
	return fputc(c, u4f->file);
}

int zu4_file_zip_putc(U4FILE *u4f, int c) {
	zu4_assert(0, "zipfiles must be read-only!");
	return c;
}

int zu4_file_getshort(U4FILE *u4f) {
	int byteLow = zu4_file_getc(u4f);
	return byteLow | (zu4_file_getc(u4f) << 8);
}
