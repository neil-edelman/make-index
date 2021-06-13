/** @license 2000, 2012 Neil Edelman, distributed under the terms of the
 [GNU General Public License 3](https://opensource.org/licenses/GPL-3.0).

 @subtitle make-index
 @author Neil

 `Recursor` is the main part of `make-index`, a content management system that
 generates static content on all the directories based on templates rooted at
 the working directory.

 There should be an `example` directory that has a bunch of files in it. Run
 `../bin/make-index` in the example directory; it should make a webpage out of
 the directory structure and the templates.

 @std POSIX.1
 @fixme Borrow `Cdoc` parser for the `.d` files. (`lex` is pretty good.)
 @fixme Encoding is an issue; especially the newsfeed, which requires 7-bit.
 @fixme It's not robust; eg @(files){@(files){Don't do this.}}. */

#include <stdlib.h>		/* malloc free fgets */
#include <stdio.h>		/* fprintf FILE */
#include <string.h>		/* strcmp */
#include <unistd.h>		/* chdir (POSIX, not ANSI) */
#include <sys/types.h>	/* mode_t (umask) */
#include <sys/stat.h>	/* umask */
#include <errno.h>		/* EDOM */
#include <assert.h>
#include "Files.h"
#include "Widget.h"
#include "Parser.h"

/* constants */
static const size_t granularity      = 1024;
static const char *html_index        = "index.html";
static const char *xml_sitemap       = "sitemap.xml";
static const char *rss_newsfeed      = "newsfeed.rss";
static const char *template_index    = ".index.html";
static const char *template_sitemap  = ".sitemap.xml";
static const char *template_newsfeed = ".newsfeed.rss";
/* in Files.c */
extern const char *dir_current;
extern const char *dir_parent;
/* in Widget.c */
extern const char *dot_news;

static const char *why;

/* Singleton. */
static struct recursor {
	struct { char *string; struct Parser *parser; } index;
	struct { char *string; struct Parser *parser; FILE *fp; } sitemap, newsfeed;
} *r;

/* private */

static void usage(void) {
	static const char *programme = "make-index";
	fprintf(stderr,
		"%s is a content management system that generates static\n"
		"content on all the directories rooted at the current directory.\n\n"
		"If you have these files accessible in the current directory, then,\n"
		"<%s>\tcreates <%s> in all accessible subdirectories,\n"
		"<%s>\tcreates <%s> from all the .news encountered,\n"
		"<%s>\tcreates <%s> of all accessible subdirectories.\n\n"
		"Of special signifigance:\n"
		" <file>.d is a description of <file>;\n"
		"  if this description is an empty, it skips over this file;\n"
		" index.d is a description of the directory;\n"
		" content.d is an in-depth description of the directory;\n"
		" <file>.d.jpg is an (icon) image that will go with the description;\n"
		" <news>.news as a newsworthy item; the format of this file is\n"
		"  ISO 8601 date (YYYY-MM-DD,) next line title;\n"
		" <link>.link as a link with the href in the file.\n",
		programme,
		template_index, html_index,
		template_newsfeed, rss_newsfeed,
		template_sitemap, xml_sitemap);
	fprintf(stderr,
		"2000, 2012 Neil Edelman, distributed under the terms of the\n"
		"GNU General Public License 3.\n\n");
}

/** This is not that great for error handling.
 @return Reads the entire `filename` and sticks it into a dynamically
 allocated string. One must `free` the memory. */
static char *readFile(const char *filename) {
	char *buf = 0, *newBuf;
	size_t bufPos = 0, bufSize = 0, rd;
	FILE *fp;
	if(!filename) return 0;
	if(!(fp = fopen(filename, "r"))) { perror(filename); return 0; }
	for( ; ; ) {
		newBuf = realloc(buf, (bufSize += granularity) * sizeof(char));
		if(!newBuf) { perror(filename); free(buf); return 0; }
		buf     = newBuf;
		rd      = fread(buf + bufPos, sizeof(char), granularity, fp);
		bufPos += rd;
		if(rd < granularity) { buf[bufPos] = '\0'; break; }
	}
	fprintf(stderr, "Opened '%s' and alloted %lu bytes to read %lu bytes.\n",
		filename, bufSize, bufPos);
	if(fclose(fp)) perror(filename);
	return buf;
}

/** Destructor. */
static void recursor_(void) {
	if(!r) return;
	if(r->sitemap.parser && r->sitemap.fp)  {
		ParserParse(r->sitemap.parser,  0, -1, r->sitemap.fp);
		ParserParse(r->sitemap.parser,  0, 0,  r->sitemap.fp);
	}
	if(r->sitemap.fp && fclose(r->sitemap.fp)) perror(xml_sitemap);
	if(r->newsfeed.parser && r->newsfeed.fp) {
		ParserParse(r->newsfeed.parser, 0, -1, r->newsfeed.fp);
		ParserParse(r->newsfeed.parser, 0, 0,  r->newsfeed.fp);
	}
	if(r->newsfeed.fp && fclose(r->newsfeed.fp)) perror(rss_newsfeed);
	Parser_(&r->index.parser);
	free(r->index.string);
	Parser_(&r->sitemap.parser);
	free(r->sitemap.string);
	Parser_(&r->newsfeed.parser);
	free(r->newsfeed.string);
	free(r);
	r = 0;
}

/** Constructor of singleton. */
static struct recursor *recursor(void) {
	assert(!r);
	if(!(r = malloc(sizeof *r))) { why = "recursor"; goto catch; };
	r->index.string = 0;
	r->index.parser = 0;
	r->sitemap.string = 0;
	r->sitemap.parser = 0;
	r->sitemap.fp = 0;
	r->newsfeed.string = 0;
	r->newsfeed.parser = 0;
	r->newsfeed.fp = 0;
	/* read index template -- index is opened multiple times */
	if(!(r->index.string = readFile(template_index))) {
		fprintf(stderr, "Recursor: to make an index, create the file <%s>.\n",
			template_index);
	} else {
		if(!(r->index.parser = Parser(r->index.string)))
			{ why = template_index; goto catch; }
	}

	/* read sitemap template */
	if(!(r->sitemap.string = readFile(template_sitemap))) {
		fprintf(stderr, "Recursor: to make an sitemap, create the file <%s>.\n",
			template_sitemap);
	} else {
		if(!(r->sitemap.fp = fopen(xml_sitemap,  "w")))
			{ why = xml_sitemap; goto catch; }
		if(!(r->sitemap.parser = Parser(r->sitemap.string)))
			{ why = template_sitemap; goto catch; }
	}

	/* read newsfeed template */
	if(!(r->newsfeed.string = readFile(template_newsfeed))) {
		fprintf(stderr, "Recursor: to make a newsfeed, create the file <%s>.\n",
			template_newsfeed);
	} else {
		if(!(r->newsfeed.fp = fopen(rss_newsfeed, "w")))
			{ why = rss_newsfeed; goto catch; }
		if(!(r->newsfeed.parser = Parser(r->newsfeed.string))) {
			{ why = template_newsfeed; goto catch; }
		}
	}

	/* if there's no content, we have nothing to do */
	if(!r->index.parser && !r->sitemap.parser && !r->newsfeed.parser)
		{ why = "no parsers"; errno = EDOM; goto catch; }

	/* parse the "header," ie, everything up to ~, the second arg is null
	 because we haven't set up the Files, so @files{}, @pwd{}, etc are
	 undefined */
	ParserParse(r->sitemap.parser,  0, 0, r->sitemap.fp);
	ParserParse(r->newsfeed.parser, 0, 0, r->newsfeed.fp);
	goto finally;
catch:
	recursor_();
finally:
	return r;
}

/** @return Binary value that says if `files` say `fn` should be included.
 @implements FilesFilter */
static int filter(struct Files *const files, const char *fn) {
	const char *str;
	char filed[64];
	FILE *fd;
	assert(r);
	/* *.d[.0]* */
	for(str = fn; (str = strstr(str, dot_desc)); ) {
		str += strlen(dot_desc);
		if(*str == '\0' || *str == '.') return 0;
	}
	/* *.news$ */
	if((str = strstr(fn, dot_news))) {
		str += strlen(dot_news);
		if(*str == '\0') {
			if(WidgetSetNews(fn)
				&& ParserParse(r->newsfeed.parser, files, 0, r->newsfeed.fp)) {
				ParserRewind(r->newsfeed.parser);
			} else {
				fprintf(stderr, "Recursor::filter: error writing news <%s>.\n", fn);
			}
			return 0;
		}
	}
	/* . */
	if(!strcmp(fn, dir_current)) return 0;
	/* .. */
	if(!strcmp(fn, dir_parent) && FilesIsRoot(files)) return 0;
	/* index.html */
	if(!strcmp(fn, html_index))  return 0;
	/* add .d, check 1 line for \n (hmm, this must be a real time waster) */
	if(strlen(fn) > sizeof(filed) - strlen(dot_desc) - 1)
		return fprintf(stderr,
		"Recusor::filter: regected '%s' because it was too long (%d.)\n",
		fn, (int)sizeof(filed)), 0;
	strcpy(filed, fn);
	strcat(filed, dot_desc);
	if((fd = fopen(filed, "r"))) {
		int ch = fgetc(fd);
		if(ch == '\n' || ch == '\r' || ch == EOF)
			return fprintf(stderr, "Recursor::filter: '%s' rejected.\n", fn), 0;
		if(fclose(fd)) perror(filed);
	}
	return 1;
}

/** Called recursively with `parent` initially set to null. @return True. */
static int recurse(struct Files *const parent) {
	struct Files *f;
	const char   *name;
	FILE         *fp;
	f = Files(parent, &filter);
	/* write the index */
	if((fp = fopen(html_index, "w"))) {
		ParserParse(r->index.parser, f, 0, fp);
		ParserRewind(r->index.parser);
		fclose(fp);
	} else perror(html_index); /* fixme: this should be an error */
	/* sitemap */
	ParserParse(r->sitemap.parser, f, 0, r->sitemap.fp);
	ParserRewind(r->sitemap.parser);
	/* recurse */
	while(FilesAdvance(f)) {
		if(!FilesIsDir(f) ||
		   !(name = FilesName(f)) ||
		   !strcmp(dir_current, name) ||
		   !strcmp(dir_parent,  name) ||
		   !(name = FilesName(f))) continue;
		if(chdir(name)) { perror(name); continue; }
		recurse(f);
		/* this happens on Windows; I don't know what to do */
		if(chdir(dir_parent)) perror(dir_parent);
	}
	Files_(f);
	return 1;
}

/** Make sure that `argc`, `argv`, aren't expecting user input. */
int main(int argc, char **argv) {
	int ret = EXIT_FAILURE;
	(void)argv;
	if(argc != 1) { why = "arguments"; errno = EDOM; goto catch; }
	/* make sure that umask is set so that others can read what we create */
	umask((mode_t)(S_IWGRP | S_IWOTH));
	/* recursing */
	if(!recursor() || !recurse(0)) goto catch;
	ret = EXIT_SUCCESS;
	goto finally;
catch:
	perror(why);
	fputc('\n', stdout);
	usage();
finally:
	recursor_();
	return ret;
}
