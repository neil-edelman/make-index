/** Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt

 {Recursor} is the main part of {MakeIndex}, a content management system that
 generates static content, (mostly index.html,) on all the directories rooted
 at the directory specified by {--directory} or {-d}. It is based on a template
 file, {.index.html} and {.newsfeed.rss}, which are changeable. Also included
 are files to summarise the directory structure for a {xml} site map,
 compatible with Google, and any {.news} for an {rss} feed.

 There should be an <example> directory that has a bunch of files in it. Run
 {bin/MakeIndex -d example/}; it should make a webpage out of the directory
 structure and {.index.html}, open {example/index.html} after running to see.

 * If the {.index.html} file exists in the <directory>, prints <index.html>
   recursively; overwrites any {index.html} on all the directories rooted at
   <directory>;
 * if the {.sitemap.xml} file exists in <directory>, prints (and overwrites) an
   index called {sitemap.xml};
 * if the {.newsfeed.rss} file exists in <directory>, prints (and overwrites)
   to {newsfeed.rss} all the {.news} files (if there are any.)

 * Treats {.d} as a description of the file without the {.d};
   if this is an empty text-file or a zero-byte file, it skips over this file.
 * treats {index.d} as a description of the directory;
 * treats {content.d} as an in-depth description of the directory,
   replacing <index.d> when in the directory;
 * treats {.d.jpg} as a image that will go with the description;
 * treats {.news} as a newsworthy item; the format of this file is ISO 8601
   date (YYYY-MM-DD,) next line title;
 * treats {.link} as a link with the href in the file.

 {.index.html}, {.sitemap.xml}, {.newsfeed.rss}, see {Parser} for recognised
 symbols. Assumes '..' is the parent directory, '.' is the current directory,
 and '/' is the directory separator; works for UNIX, MacOS, and Windows. If
 this is not the case, the constants are in {Files.c}.

 @title		Recursor
 @author	Neil
 @std		POSIX.1
 @version	2018-03 Removed version numbers.
 @since		1.1; 2017-03 Fixed pedantic warnings; command-line improvements.
			1.0; 2016-09-19 Added umask.
			0.8; 2013-07 Case-insensitive sort.
			0.7; 2012    {sth.dsth.d} handled properly.
			0.6; 2008-03-27 Made something out of an earlier work.
 @fixme		Don't have <directory> be an argument; just do it in the current.
 @fixme		Borrow {Cdoc} parser for the {.d} files.
 @fixme		Encoding is an issue; especially the newsfeed, which requires 7-bit.
 @fixme		It's not robust; eg @(files){@(files){Don't do this.}}.
 @fixme		Simplify the command line arguments. */

#include <stdlib.h>		/* malloc free fgets */
#include <stdio.h>		/* fprintf FILE */
#include <string.h>		/* strcmp */
#include <unistd.h>		/* chdir (POSIX, not ANSI) */
#include <sys/types.h>	/* mode_t (umask) */
#include <sys/stat.h>	/* umask */
#include "Files.h"
#include "Widget.h"
#include "Parser.h"
#include "Recursor.h"

/* constants */
static const char *const programme = "MakeIndex";
static const int version_major     = 1;
static const int version_minor     = 1;

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
extern const char *dot_desc;
extern const char *dot_news;

/* public */
struct Recursor {
	char          *indexString;
	struct Parser *indexParser;    /* depends on indexString */
	FILE          *sitemap;
	char          *sitemapString;
	struct Parser *sitemapParser;  /* depends on sitemapString */
	FILE          *newsfeed;
	char          *newsfeedString;
	struct Parser *newsfeedParser; /* depends on newsfeedString */
};

/* private */
static int filter(struct Files *const files, const char *fn);
static int recurse(struct Files *const parent);
static char *readFile(const char *filename);
static void usage(void);

/* there can only be one recursor at a time, sorry */
static struct Recursor *r = 0;

/* public */

/** 
 @param idx: file name of the prototype index file, eg, ".index.html".
 @param map: file name of the prototype map file, eg, ".sitmap.xml".
 @param news: file name of the prototype news file, eg, ".newsfeed.rss". */
struct Recursor *Recursor(const char *idx, const char *map, const char *news) {
	if(!idx || !idx || !map || !news) return 0;
	if(r) { fprintf(stderr, "Recursor: there is already a Recursor.\n");
		return 0; }
	r = malloc(sizeof(struct Recursor));
	if(!r) { perror("recursor"); Recursor_(); return 0; }
	r->indexString    = 0;
	r->indexParser    = 0;
	r->sitemap        = 0;
	r->sitemapString  = 0;
	r->sitemapParser  = 0;
	r->newsfeed       = 0;
	r->newsfeedString = 0;
	r->newsfeedParser = 0;
	/* open the files for writing (index is opened multiple times in the directories) */
	if(!(r->sitemap  = fopen(xml_sitemap,  "w"))) perror(xml_sitemap);
	if(!(r->newsfeed = fopen(rss_newsfeed, "w"))) perror(rss_newsfeed);
	/* read from the input files */
	if(                !(r->indexString   = readFile(idx))) {
		fprintf(stderr, "Recursor: to make an index, create the file <%s>.\n",
			idx);
	}
	if(r->sitemap  && !(r->sitemapString  = readFile(map))) {
		fprintf(stderr, "Recursor: to make an sitemap, create the file <%s>.\n",
			map);
	}
	if(r->newsfeed && !(r->newsfeedString = readFile(news))) {
		fprintf(stderr, "Recursor: to make a newsfeed, create the file <%s>.\n",
			news);
	}
	/* create Parsers attached to them */
	if(r->indexString    && !(r->indexParser    = Parser(r->indexString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", idx);
	}
	if(r->sitemapString  && !(r->sitemapParser  = Parser(r->sitemapString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", map);
	}
	if(r->newsfeedString && !(r->newsfeedParser = Parser(r->newsfeedString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", news);
	}
	/* if theirs no content, we have nothing to do */
	if(!r->indexParser && !r->sitemapParser && !r->newsfeedParser) {
		fprintf(stderr, "Recursor: no Parsers defined, it would be useless to "
			"continue.\n");
		Recursor_();
		return 0;
	}
	/* parse the "header," ie, everything up to ~, the second arg is null
	 because we haven't set up the Files, so @files{}, @pwd{}, etc are
	 undefined */
	ParserParse(r->sitemapParser,  0, 0, r->sitemap);
	ParserParse(r->newsfeedParser, 0, 0, r->newsfeed);
	return r;
}

/** Destructor. */
void Recursor_(void) {
	if(!r) return;
	if(r->sitemapParser  && r->sitemap)  {
		ParserParse(r->sitemapParser,  0, -1, r->sitemap);
		ParserParse(r->sitemapParser,  0, 0,  r->sitemap);
	}
	if(r->sitemap  && fclose(r->sitemap))  perror(xml_sitemap);
	if(r->newsfeedParser && r->newsfeed) {
		ParserParse(r->newsfeedParser, 0, -1, r->newsfeed);
		ParserParse(r->newsfeedParser, 0, 0,  r->newsfeed);
	}
	if(r->newsfeed && fclose(r->newsfeed)) perror(rss_newsfeed);
	Parser_(&r->indexParser);
	free(r->indexString);
	Parser_(&r->sitemapParser);
	free(r->sitemapString);
	Parser_(&r->newsfeedParser);
	free(r->newsfeedString);	
	free(r);
	r = 0;
}

/** Actually does the recursion. */
int RecursorGo(void) {
	if(!r) return 0;
	return recurse(0);
}

/* private */

/** @implements FilesFilter */
static int filter(struct Files *const files, const char *fn) {
	const char *str;
	char filed[64];
	FILE *fd;
	if(!r) { fprintf(stderr, "Recusor::filter: recursor not initialised.\n");
		return 0; }
	/* *.d[.0]* */
	for(str = fn; (str = strstr(str, dot_desc)); ) {
		str += strlen(dot_desc);
		if(*str == '\0' || *str == '.') return 0;
	}
	/* *.news$ */
	if((str = strstr(fn, dot_news))) {
		str += strlen(dot_news);
		if(*str == '\0') {
			if(WidgetSetNews(fn) && ParserParse(r->newsfeedParser, files, 0, r->newsfeed)) {
				ParserRewind(r->newsfeedParser);
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
	if(strlen(fn) > sizeof(filed) - strlen(dot_desc) - 1) {
		fprintf(stderr, "Recusor::filter: regected '%s' because it was too long (%d.)\n", fn, (int)sizeof(filed));
		return 0;
	}
	strcpy(filed, fn);
	strcat(filed, dot_desc);
	if((fd = fopen(filed, "r"))) {
		int ch = fgetc(fd);
		if(ch == '\n' || ch == '\r' || ch == EOF) {
			fprintf(stderr, "Recursor::filter: '%s' rejected.\n", fn);
			return 0;
		}
		if(fclose(fd)) perror(filed);
	}
	return -1;
}

static int recurse(struct Files *const parent) {
	struct Files *f;
	const char   *name;
	FILE         *fp;
	f = Files(parent, &filter);
	/* write the index */
	if((fp = fopen(html_index, "w"))) {
		ParserParse(r->indexParser, f, 0, fp);
		ParserRewind(r->indexParser);
		fclose(fp);
	} else perror(html_index);
	/* sitemap */
	ParserParse(r->sitemapParser, f, 0, r->sitemap);
	ParserRewind(r->sitemapParser);
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
	return -1;
}

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
	fprintf(stderr, "Opened '%s' and alloted %lu bytes to read %lu "
		"characters.\n", filename, bufSize, bufPos);
	if(fclose(fp)) perror(filename);
	return buf; /* you must free() the memory! */
}

static void usage(void) {
	fprintf(stderr, "Usage: %s [--directory|-d] <directory> | [\n"
		"\t  [--input-index   |-ti] <.index.html>\n"
		"\t| [--input-newsfeed|-tn] <.newsfeed.rss>\n"
		"\t| [--input-sitemap |-ts] <.sitemap.xml>\n"
		"\t| [--output-index   |-i] <index.html>\n"
		"\t| [--output-newsfeed|-n] <newsfeed.rss>\n"
		"\t| [--output-sitemap |-s] <sitemap.xml>\n"
		"] | [--help|-h]\n\n", programme);
	fprintf(stderr, "All the defaults are listed, except --directory, which "
		"requires a value.\n"
		"Input are in <directory>.\n\n"
		"Version %d.%d.\n\n"
		"MakeIndex is a content management system that generates static\n"
		"content, (mostly index.html,) on all the directories rooted at the\n"
		"<directory> based on <%s>.\n\n"
		"It also does some other stuff. See readme.txt or\n"
		"http://neil.chaosnet.org/ for further info.\n\n",
		version_major, version_minor, template_index);
	fprintf(stderr, "MakeIndex Copyright 2008, 2012 Neil Edelman\n"
		"This program comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and you are welcome to redistribute it\n"
			"under certain conditions; see gpl.txt.\n\n");
}

/* Entry-point (shouldn't have a prototype.) */
int main(int argc, char **argv) {
	int ac, ret;
	int is_help = 0, is_invalid = 0;
	const char *directory = 0;

	/* gperf is wonderful, but what about other build systems? */
	for(ac = 1; ac < argc; ac++) {
		const char *const av = argv[ac];
		if(!strcmp("--help", av) || !strcmp("-h", av)) {
			is_help = -1;
		} else if(!strcmp("--directory", av) || !strcmp("-d", av)) {
			if(directory) {
				is_invalid = -1; break;
			} else if(++ac < argc) {
				directory = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--input-index", av) || !strcmp("-ii", av)) {
			if(++ac < argc) {
				template_index = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--input-newsfeed", av) || !strcmp("-in", av)) {
			if(++ac < argc) {
				template_newsfeed = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--input-sitemap", av) || !strcmp("-is", av)) {
			if(++ac < argc) {
				template_sitemap = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--output-index", av) || !strcmp("-oi", av)) {
			if(++ac < argc) {
				html_index = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--output-newsfeed", av) || !strcmp("-on", av)) {
			if(++ac < argc) {
				rss_newsfeed = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		} else if(!strcmp("--output-sitemap", av) || !strcmp("-os", av)) {
			if(++ac < argc) {
				xml_sitemap = argv[ac];
			} else {
				is_invalid = -1; break;
			}
		}		
	}

	if(is_help || !directory) {
		usage();
		return is_help || !is_invalid ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	fprintf(stderr, "Changing directory to <%s>.\n", directory);
	if(chdir(directory)) { perror(directory); return EXIT_FAILURE; }

	/* make sure that umask is set so that others can read what we create
	 "warning: passing argument 1 of 'umask' with different width due to
	 prototype" <- umask's fault; can't change? */
	umask(S_IWGRP | S_IWOTH);
	
	/* recursing; fixme: this should be configurable */
	if(!Recursor(template_index, template_sitemap, template_newsfeed))
		return EXIT_FAILURE;
	ret = RecursorGo();
	Recursor_();
	return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
