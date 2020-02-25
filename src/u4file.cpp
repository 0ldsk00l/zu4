/*
 * u4file.cpp
 * Copyright (C) 2014 Darren Janeczek
 * Copyright (C) 2019 R. Danbrook
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
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "u4file.h"
#include "error.h"

#include "miniz.h"

using std::map;
using std::string;
using std::vector;

static struct _u4paths {
	const char *rootpath;
	const char *dospath;
	const char *zippath;
	const char *confpath;
	const char *graphicspath;
} u4paths;

/**
 * A specialization of U4FILE that uses C stdio internally.
 */
class U4FILE_stdio : public U4FILE {
public:
	static U4FILE *open(const string &fname);
	
	virtual void close();
	virtual int seek(long offset, int whence);
	virtual long tell();
	virtual size_t read(void *ptr, size_t size, size_t nmemb);
	virtual int getc();
	virtual int putc(int c);
	virtual long length();
	
private:
	FILE *file;
};

/**
 * A specialization of U4FILE that reads files out of zip archives
 * automatically.
 */
class U4FILE_zip : public U4FILE {
public:
	static U4FILE *open(const string &fname, const U4ZipPackage *package);
	
	virtual void close();
	virtual int seek(long offset, int whence);
	virtual long tell();
	virtual size_t read(void *ptr, size_t size, size_t nmemb);
	virtual int getc();
	virtual int putc(int c);
	virtual long length();
	
private:
	mz_zip_archive zip_archive;
	mz_zip_archive_file_stat za_stat;
	int za_index;
	void *fptr;
	long cur;
};

extern bool verbose;

void xu4_file_set_paths() {
	u4paths = { "./", "ultima4", "./", "conf", "graphics" };
}

U4PATH * U4PATH::instance = NULL;
U4PATH * U4PATH::getInstance() {
	if (!instance) {
		instance = new U4PATH();
		instance->initDefaultPaths();
	}
	return instance;
}

void U4PATH::initDefaultPaths() {
	if (defaultsHaveBeenInitd) { return; }
	
	//The first part of the path searched will be one of these root directories
	
	/*Try to cover all root possibilities. These can be added to by separate modules*/
	rootResourcePaths.push_back("./");
	
	//The second (specific) part of the path searched will be these various subdirectories
	
	/* the possible paths where u4 for DOS can be installed */
	u4ForDOSPaths.push_back("ultima4");
	
	/* the possible paths where the u4 zipfiles can be installed */
	u4ZipPaths.push_back("./");
	
	/* the possible paths where the u4 config files can be installed */
	configPaths.push_back("conf");
	
	/* the possible paths where the u4 graphics files can be installed */
	graphicsPaths.push_back("graphics");
}

/**
 * Returns true if the upgrade is present.
 */
bool u4isUpgradeAvailable() {
	bool avail = false;
	U4FILE *pal;
	if ((pal = u4fopen("u4vga.pal")) != NULL) {
		avail = true;
		u4fclose(pal);
	}
	return avail;
}

/**
 * Returns true if the upgrade is not only present, but is installed
 * (switch.bat or setup.bat has been run)
 */
bool u4isUpgradeInstalled() {
	U4FILE *u4f = NULL;
	long int filelength;
	bool result = false;
	
	/* FIXME: Is there a better way to determine this? */
	u4f = u4fopen("ega.drv");
	if (u4f) {
		filelength = u4f->length();
		u4fclose(u4f);
		/* see if (ega.drv > 5k).  If so, the upgrade is installed */
		if (filelength > (5 * 1024)) { result = true; }
	}
	
	if (verbose) { printf("u4isUpgradeInstalled %d\n", (int) result); }
	
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
	string upg_pathname(u4find_path("u4upgrad.zip", u4paths.zippath));
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
	int flag = 0;
	string pathname;
	
	// We check for all manner of generic packages, though.
	pathname = u4find_path("ultima4.zip", u4paths.zippath);
	if (!pathname.empty()) {
		flag = 1;
	}

	if (flag) {
		// Open the zip
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		
		// Check zip file validity
		if (!mz_zip_reader_init_file(&zip_archive, pathname.c_str(), 0)) {
			xu4_error(XU4_LOG_ERR, "Archive corrupt, exiting...\n");
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

int U4FILE::getshort() {
	int byteLow = getc();
	return byteLow | (getc() << 8);
}

U4FILE *U4FILE_stdio::open(const string &fname) {
	U4FILE_stdio *u4f;
	FILE *f;

	f = fopen(fname.c_str(), "rb");
	if (!f) { return NULL; }

	u4f = new U4FILE_stdio;
	u4f->file = f;
	
	return u4f;
}

void U4FILE_stdio::close() {
	fclose(file);
}

int U4FILE_stdio::seek(long offset, int whence) {
	return fseek(file, offset, whence);
}

long U4FILE_stdio::tell() {
	return ftell(file);
}

size_t U4FILE_stdio::read(void *ptr, size_t size, size_t nmemb) {
	return fread(ptr, size, nmemb, file);
}

int U4FILE_stdio::getc() {
	return fgetc(file);
}

int U4FILE_stdio::putc(int c) {
	return fputc(c, file);
}

long U4FILE_stdio::length() {
	long curr, len;
	
	curr = ftell(file);
	fseek(file, 0L, SEEK_END);
	len = ftell(file);
	fseek(file, curr, SEEK_SET);
	
	return len;
}

/**
 * Opens a file from within a zip archive.
 */
U4FILE *U4FILE_zip::open(const string &fname, const U4ZipPackage *package) {
	U4FILE_zip *u4f;
	
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
	
	u4f = new U4FILE_zip;
	u4f->zip_archive = za;
	u4f->za_index = index;
	u4f->za_stat = stat;
	u4f->fptr = ptr;
	u4f->cur = 0;
	
	return u4f;
}

void U4FILE_zip::close() {
	mz_zip_reader_end(&zip_archive);
	free(fptr);
}

int U4FILE_zip::seek(long offset, int whence) {
	long pos;
	
	xu4_assert(whence != SEEK_END, "seeking with whence == SEEK_END not allowed with zipfiles");
	pos = cur;
	
	if (whence == SEEK_CUR) {
		offset = pos + offset;
	}
	
	if (offset == pos) {
		return 0;
	}
	
	if (offset < pos) {
		pos = 0;
		cur = 0;
	}
	
	xu4_assert(offset - pos > 0, "error in U4FILE_zip::seek");
	cur += offset - pos;
	return 0;
}

long U4FILE_zip::tell() {
	return cur;
}

size_t U4FILE_zip::read(void *ptr, size_t size, size_t nmemb) {
	size_t retval = nmemb * size;
	if (retval) {
		unsigned char *temp = (unsigned char*)fptr;
		memcpy(ptr, temp + cur, retval);
		cur += retval;
	}
	return retval;
}

int U4FILE_zip::getc() {
	unsigned char *retptr = (unsigned char*)fptr;
	return (int)retptr[cur++];
}

int U4FILE_zip::putc(int c) {
	xu4_assert(0, "zipfiles must be read-only!");
	return c;
}

long U4FILE_zip::length() {
	return za_stat.m_uncomp_size;
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
U4FILE *u4fopen(const string &fname) {
	U4FILE *u4f = NULL;
	unsigned int i;
	
	if (verbose) {
		printf("looking for %s\n", fname.c_str());
	}
	
	/**
	 * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
	 */
	const vector<U4ZipPackage *> &packages = U4ZipPackageMgr::getInstance()->getPackages(); 
	for (std::vector<U4ZipPackage *>::const_reverse_iterator j = packages.rbegin(); j != packages.rend(); j++) {
		u4f = U4FILE_zip::open(fname, *j);
		if (u4f)
			return u4f; /* file was found, return it! */
	}
	
	/*
	 * file not in a zipfile; check if it has been unzipped
	 */
	string fname_copy(fname);
	
	string pathname = u4find_path(fname_copy.c_str(), u4paths.dospath);
	if (pathname.empty()) {
		using namespace std;
		if (islower(fname_copy[0])) {
			fname_copy[0] = toupper(fname_copy[0]);
			pathname = u4find_path(fname_copy.c_str(), u4paths.dospath);
		}
		
		if (pathname.empty()) {
			for (i = 0; fname_copy[i] != '\0'; i++) {
				if (islower(fname_copy[i]))
					fname_copy[i] = toupper(fname_copy[i]);
			}
			pathname = u4find_path(fname_copy.c_str(), u4paths.dospath);
		}
	}
	
	if (!pathname.empty()) {
		u4f = U4FILE_stdio::open(pathname);
		if (verbose && u4f != NULL) {
			printf("%s successfully opened\n", pathname.c_str());
		}
	}
	
	return u4f;
}

/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE.
 */
U4FILE *u4fopen_stdio(const string &fname) {
	return U4FILE_stdio::open(fname);
}

/**
 * Opens a file from a zipfile and wraps it in a U4FILE.
 */
U4FILE *u4fopen_zip(const string &fname, U4ZipPackage *package) {
	return U4FILE_zip::open(fname, package);
}

/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(U4FILE *f) {
	f->close();
	delete f;
}

int u4fseek(U4FILE *f, long offset, int whence) {
	return f->seek(offset, whence);
}

long u4ftell(U4FILE *f) {
	return f->tell();
}

size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f) {
	return f->read(ptr, size, nmemb);
}

int u4fgetc(U4FILE *f) {
	return f->getc();
}

int u4fgetshort(U4FILE *f) {
	return f->getshort();
}

int u4fputc(int c, U4FILE *f) {
	return f->putc(c);
}

/**
 * Returns the length in bytes of a file.
 */
long u4flength(U4FILE *f) {
	return f->length();
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

	xu4_assert(offset < u4flength(f), "offset begins beyond end of file");
	
	if (offset != -1) { f->seek(offset, SEEK_SET); }
	
	for (i = 0; i < nstrings; i++) {
		char c;
		buffer.erase();

		while ((c = f->getc()) != '\0')
			buffer += c;
		
		strs.push_back(buffer);
	}
	
	return strs;
}

string u4find_path(const char *fname, const char *subpath) {
	FILE *f = NULL;
	char path[256];
	
	f = fopen(fname, "rb");
	if (f) { snprintf(path, sizeof(path), "%s", fname); }
	
	// Try paths
	if (f == NULL) {
		snprintf(path, sizeof(path), "%s/%s/%s", u4paths.rootpath, subpath, fname);
		
		if (verbose) {
			printf("trying to open %s\n", path);
		}
		if ((f = fopen(path, "rb")) != NULL) {
			xu4_error(XU4_LOG_DBG, "u4file opened %s\n", path);
		}
	}
	
	if (verbose) {
		if (f != NULL) { printf("%s successfully found\n", path); }
		else { printf("%s not found\n", fname); }
	}
	
	if (f) {
		fclose(f);
		return path;
	}
	else { return ""; }
}

string u4find_conf(const string &fname) {
	return u4find_path(fname.c_str(), u4paths.confpath);
}

string u4find_graphics(const string &fname) {
	return u4find_path(fname.c_str(), u4paths.graphicspath);
}
