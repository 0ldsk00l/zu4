/*
 * u4file.cpp
 * Copyright (C) 2014 xu4 Team
 * Copyright (C) 2020 R. Danbrook
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

using std::map;
using std::string;
using std::vector;

static struct _u4paths {
	const char *rootpath = "./";
	const char *dospath = "ultima4";
	const char *zippath = "./";
	const char *confpath = "conf";
	const char *graphicspath = "graphics";
} u4paths;

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
 * Creates a new zip package.
 */
U4ZipPackage::U4ZipPackage(const string &name, const string &path, bool extension) {
	this->name = name;
	this->path = path;
	this->extension = extension;
}

void U4ZipPackage::addTranslation(const string &value, const string &translation) {
	translations[value] = translation;
}
	
const string &U4ZipPackage::translate(const string &name) const {
	std::map<string, string>::const_iterator i = translations.find(name);
	if (i != translations.end()) { return i->second; }
	else { return name; }
}

U4ZipPackageMgr *U4ZipPackageMgr::instance = NULL;

U4ZipPackageMgr *U4ZipPackageMgr::getInstance() {
	if (instance == NULL) {
		instance = new U4ZipPackageMgr();
	}
	return instance;
}

void U4ZipPackageMgr::destroy() {
	if (instance != NULL) {
		delete instance;
		instance = NULL;
	}
}
	
void U4ZipPackageMgr::add(U4ZipPackage *package) {
	packages.push_back(package);
}

U4ZipPackageMgr::U4ZipPackageMgr() {
	char upgpath[64];
	u4find_path(upgpath, sizeof(upgpath), "u4upgrad.zip", u4paths.zippath);
	string upg_pathname = (string)upgpath;
	if (!upg_pathname.empty()) {
		/* upgrade zip is present */
		U4ZipPackage *upgrade = new U4ZipPackage(upg_pathname, "", false);
		upgrade->addTranslation("compassn.ega", "compassn.old");
		upgrade->addTranslation("courage.ega", "courage.old");
		upgrade->addTranslation("cove.tlk", "cove.old");
		upgrade->addTranslation("ega.drv", "ega.old"); // not actually used
		upgrade->addTranslation("honesty.ega", "honesty.old");
		upgrade->addTranslation("honor.ega", "honor.old");
		upgrade->addTranslation("humility.ega", "humility.old");
		upgrade->addTranslation("key7.ega", "key7.old");
		upgrade->addTranslation("lcb.tlk", "lcb.old");
		upgrade->addTranslation("love.ega", "love.old");
		upgrade->addTranslation("love.ega", "love.old");
		upgrade->addTranslation("minoc.tlk", "minoc.old");
		upgrade->addTranslation("rune_0.ega", "rune_0.old");
		upgrade->addTranslation("rune_1.ega", "rune_1.old");
		upgrade->addTranslation("rune_2.ega", "rune_2.old");
		upgrade->addTranslation("rune_3.ega", "rune_3.old");
		upgrade->addTranslation("rune_4.ega", "rune_4.old");
		upgrade->addTranslation("rune_5.ega", "rune_5.old");
		upgrade->addTranslation("sacrific.ega", "sacrific.old");
		upgrade->addTranslation("skara.tlk", "skara.old");
		upgrade->addTranslation("spirit.ega", "spirit.old");
		upgrade->addTranslation("start.ega", "start.old");
		upgrade->addTranslation("stoncrcl.ega", "stoncrcl.old");
		upgrade->addTranslation("truth.ega", "truth.old");
		upgrade->addTranslation("ultima.com", "ultima.old"); // not actually used
		upgrade->addTranslation("valor.ega", "valor.old");
		upgrade->addTranslation("yew.tlk", "yew.old");
		add(upgrade);
	}
	
	// Check for the default zip packages
	char path[64];
	u4find_path(path, sizeof(path), "ultima4.zip", u4paths.zippath);
	string pathname = (string)path;
	
	if (!pathname.empty()) {
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
			add(new U4ZipPackage(pathname, "", false));
		}
		
		mz_zip_reader_end(&zip_archive);
	}
}

U4ZipPackageMgr::~U4ZipPackageMgr() {
	for (std::vector<U4ZipPackage *>::iterator i = packages.begin(); i != packages.end(); i++) {
		delete *i;
	}
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
	unsigned int i;
	
	zu4_error(ZU4_LOG_DBG, "looking for %s\n", fname);
	
	/**
	 * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
	 */
	const vector<U4ZipPackage *> &packages = U4ZipPackageMgr::getInstance()->getPackages(); 
	for (std::vector<U4ZipPackage *>::const_reverse_iterator j = packages.rbegin(); j != packages.rend(); j++) {
		u4f = u4fopen_zip(fname, *j);
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
	}
	
	/*
	 * file not in a zipfile; check if it has been unzipped
	 */
	string fname_copy = (string)fname;
	
	char path[64];
	u4find_path(path, sizeof(path), fname_copy.c_str(), u4paths.dospath);
	string pathname = (string)path;
	if (pathname.empty()) {
		using namespace std;
		if (islower(fname_copy[0])) {
			fname_copy[0] = toupper(fname_copy[0]);
			u4find_path(path, sizeof(path), fname_copy.c_str(), u4paths.dospath);
			pathname = (string)path;
		}
		
		if (pathname.empty()) {
			for (i = 0; fname_copy[i] != '\0'; i++) {
				if (islower(fname_copy[i]))
					fname_copy[i] = toupper(fname_copy[i]);
			}
			u4find_path(path, sizeof(path), fname_copy.c_str(), u4paths.dospath);
			pathname = (string)path;
		}
	}
	
	if (!pathname.empty()) {
		u4f = u4fopen_stdio(pathname.c_str());
		if (u4f != NULL) {
			zu4_error(ZU4_LOG_DBG, "%s successfully opened\n", pathname.c_str());
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
U4FILE *u4fopen_zip(const string &fname, U4ZipPackage *package) {
	U4FILE *u4f;
	
	mz_zip_archive za;
	memset(&za, 0, sizeof(za));
	
	// Check zip file validity
	if (!mz_zip_reader_init_file(&za, package->getFilename().c_str(), 0)) {
		return NULL;
	}
	
	string pathname = package->getInternalPath() + package->translate(fname);
	
	int index = mz_zip_reader_locate_file(&za, pathname.c_str(), NULL, 0);
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
vector<string> u4read_stringtable(U4FILE *f, long offset, int nstrings) {
	string buffer;
	int i;
	vector<string> strs;

	zu4_assert(offset < u4flength(f), "offset begins beyond end of file");
	
	if (offset != -1) { u4fseek(f, offset, SEEK_SET); }
	
	for (i = 0; i < nstrings; i++) {
		char c;
		buffer.erase();

		while ((c = zu4_file_getc(f)) != '\0')
			buffer += c;
		
		strs.push_back(buffer);
	}
	
	return strs;
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
