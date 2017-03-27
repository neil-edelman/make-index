/** Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt, or
 \url{ https://opensource.org/licenses/GPL-3.0 }.

 Files is a list of File (private class defiend below,) the Files can have
 a relation to other Files by 'parent' and 'favourite' (@(pwd) uses this.)

 @title		Files
 @author	Neil
 @std		POSIX.1
 @version	1.1; 2017-03 fixed pedantic warnings; command-line improvements
 @since		0.6; 2008-03-24 */

#include <stdlib.h>   /* malloc free */
#include <stdio.h>    /* fprintf */
#include <string.h>   /* strcmp strstr */
#include <dirent.h>   /* opendir readdir closedir */
#include <sys/stat.h> /* fstat */
#include "Files.h"

/* constants */
const char          *dir_current = "."; /* used in multiple files */
const char          *dir_parent  = "..";
static const size_t max_filename = 128;

/* public */
struct Files {
	struct Files *parent;    /* the parent, could be 0 */
	struct Files *favourite; /* temp var used to access a certain child */
	struct File  *file;      /* THIS dir, could be 0 if it's home */
	struct File  *firstFile; /* the Files in this dir */
	struct File  *firstDir;  /* the Files in this dir that are dirs */
	struct File  *this;      /* temp var, one of the firstDir, firstFile list */
};
/* private */
struct File {
	struct File *next;
	char *name;
	int size;
	int isDir;
};

/* private */
static struct File *File(const char *name, const int size, const int isDir);
static void File_(struct File *file);
static int FileInsert(struct File *file, struct File **startAddr);

/** Directory information.
 @param parent: {parent->this} must be the 'file' (directory) that you want to
 create.
 @param filter: This returns true on the files that you want included. */
struct Files *Files(struct Files *const parent, const FilesFilter filter) {
	const char    *dirName;
	struct dirent *de;
	struct stat   st;
	struct File   *file;
	struct Files  *files;
	DIR           *dir;
	if(parent && !parent->this) { fprintf(stderr, "Files: tried creating a "
		"directory without selecting a file (parent->this.)\n"); return 0; }
	files = malloc(sizeof(struct Files));
	if(!files) { perror("files"); Files_(files); return 0; }
	/* does not check for recusive dirs - assumes that it is a tree */
	files->parent    = parent;
	files->favourite = 0;
	files->file      = parent ? parent->this : 0;
	files->firstFile = 0;
	files->firstDir  = 0;
	files->this      = 0;
	/* print path on stderr */
	FilesSetPath(files);
	fprintf(stderr, "Files: directory <");
	while((dirName = FilesEnumPath(files))) fprintf(stderr, "%s/", dirName);
	fprintf(stderr, ">.\n");
	/* read the current dir */
	dir = opendir(dir_current);
	if(!dir) { perror(dir_parent); Files_(files); return 0; }
	while((de = readdir(dir))) {
		int error = 0;
		/* ignore certain files, incomplete 'files'! -> Recusor.c */
		if(!de->d_name || (filter && !filter(files, de->d_name))) continue;
		/* get status of the file */
		if(stat(de->d_name, &st)) { perror(de->d_name); continue; }
		/* get the File(name, size) (in KB) */
		file = File(de->d_name,
			((int)st.st_size + 512) >> 10, S_ISDIR(st.st_mode));
		if(!file) error = -1;
		/* put it in the appropreate spot */
		if(file->isDir) {
			if(!FileInsert(file, &files->firstDir))  error = -1;
		} else {
			if(!FileInsert(file, &files->firstFile)) error = -1;
		}
		/* error */
		if(error) fprintf(stderr, "Files: <%s> missed being included on the "
			"list.\n", de->d_name);
	}
	if(closedir(dir)) { perror(dir_current); }
	return files;
}

/** Destructor. */
void Files_(struct Files *files) {
	if(!files) return;
	File_(files->firstDir); /* cascading delete */
	File_(files->firstFile);
	free(files);
}

/** This is how we access the files sequentially. */
int FilesAdvance(struct Files *f) {
	static enum { files, dirs } type = dirs;
	if(!f) return 0;
	switch(type) {
		case dirs:
			if(!f->this) f->this = f->firstDir;
			else         f->this = f->this->next;
			if((f->this)) return -1;
			type = files;
		case files:
			if(!f->this) f->this = f->firstFile;
			else         f->this = f->this->next;
			if((f->this)) return -1;
			type = dirs;
	}
	return 0;
}

/** Doesn't have a parent? */
int FilesIsRoot(const struct Files *f) {
	if(!f) return 0;
	return (f->parent) ? 0 : -1;
}

/** Resets the list of favourites. */
void FilesSetPath(struct Files *f) {
	if(!f) return;
	for( ; f->parent; f = f->parent) f->parent->favourite = f;
}

/** @return After \see{FilesSetPath}, this enumerates them. */
const char *FilesEnumPath(struct Files *const files) {
	struct Files *f = files;
	const char *name;
	if(!f) return 0;
	for( ; f->parent && f->parent->favourite; f = f->parent);
	if(!f->favourite) return 0; /* clear favourite, done */
	name = f->favourite->file ? f->favourite->file->name : "(wtf?)";
	f->favourite = 0;
	return name;
}

/** @return The file name of the selected file. */
const char *FilesName(const struct Files *const files) {
	if(!files || !files->this) return 0;
	return files->this->name;
}

/** @return File size of the selected file in KB. */
int FilesSize(const struct Files *files) {
	if(!files || !files->this) return 0;
	return files->this->size;
}

/** @return Whether the file is a directory. */
int FilesIsDir(const struct Files *files) {
	if(!files || !files->this) return 0;
	return files->this->isDir;
}

/* private */

static struct File *File(const char *name, const int size, const int isDir) {
	size_t len;
	struct File *file;
	if(!name || !*name) { fprintf(stderr, "File: file has no name.\n");
		return 0; }
	if((len = strlen(name)) > max_filename) { fprintf(stderr, "File: file name"
		" \"%s\" is too long (%lu.)\n", name, max_filename); return 0; }
	file = malloc(sizeof(struct File) + (len + 1));
	if(!file) { File_(file); return 0; }
	file->next  = 0;
	file->name  = (char *)(file + 1);
	strncpy(file->name, name, len + 1);
	file->size  = size;
	file->isDir = isDir;
	return file;
}

static void File_(struct File *file) {
	struct File *next;
	/* delete ALL the list of files (not just the one) */
	for( ; file; file = next) {
		next = file->next;
		free(file);
	}
}

static int FileInsert(struct File *file, struct File **startAddr) {
	struct File *start;
	struct File *prev = 0, *ptr = 0;
	if(!file || !startAddr)
		{ fprintf(stderr, "FileInsert: file is null.\n"); return 0; }
	if(file->next)
		{ fprintf(stderr, "FileInsert: <%s> is already in some other list.\n",
		file->name); return 0; }
	start = (struct File *)*startAddr;
	/* 'prev' is where to insert; 'ptr' is next node */
	for(prev = 0, ptr = start; ptr; prev = ptr, ptr = ptr->next) {
		/* 4.4BSD, POSIX.1-2001 :[ */
		if(strcasecmp(file->name, ptr->name) <= 0) break;
	}
	file->next = ptr;
	if(!prev) start = file;
	else prev->next = file;
	*startAddr = start;
	return -1;
}
