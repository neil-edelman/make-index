/** Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt

 This is the main program. I didn't know what to call it.
 
 @author	Neil
 @version	1.0; 2016-09-19 Added umask
 @since		1.0; 2008-03-27 */

#include <stdlib.h>   /* malloc free fgets */
#include <stdio.h>    /* fprintf FILE */
#include <string.h>   /* strcmp */
#include <unistd.h>   /* chdir (POSIX, not ANSI) */
#include <sys/stat.h> /* umask */
#include "Files.h"
#include "Widget.h"
#include "Parser.h"
#include "Recursor.h"

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
int filter(const struct Files *, const char *fn);
int recurse(const struct Files *parent);
char *readFile(const char *filename);
void usage(const char *programme);

/* constants */
static const int versionMajor  = 0;
static const int versionMinor  = 8;
static const int granularity   = 1024;
static const int maxRead       = 0x1000;
const char *htmlIndex          = "index.html"; /* in multiple files */
static const char *xmlSitemap  = "sitemap.xml";
static const char *rssNewsfeed = "newsfeed.rss";
static const char *tmplIndex   = ".index.html";
static const char *tmplSitemap = ".sitemap.xml";
static const char *tmplNewsfeed= ".newsfeed.rss";
/* in Files.c */
extern const char *dirCurrent;
extern const char *dirParent;
/* in Widget.c */
extern const char *desc;
extern const char *news;

/* there can only be one recursor at a time, sorry */
static struct Recursor *r = 0;

/* public */
struct Recursor *Recursor(const char *index, const char *map, const char *news) {
	if(!index || !index || !map || !news) return 0;
	if(r) { fprintf(stderr, "Recursor: there is already a Recursor.\n"); return 0; }
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
	if(!(r->sitemap  = fopen(xmlSitemap,  "w"))) perror(xmlSitemap);
	if(!(r->newsfeed = fopen(rssNewsfeed, "w"))) perror(rssNewsfeed);
	/* read from the input files */
	if(                !(r->indexString   = readFile(index))) {
		fprintf(stderr, "Recursor: to make an index, create the file <%s>.\n", index);
	}
	if(r->sitemap  && !(r->sitemapString  = readFile(map))) {
		fprintf(stderr, "Recursor: to make an sitemap, create the file <%s>.\n", map);
	}
	if(r->newsfeed && !(r->newsfeedString = readFile(news))) {
		fprintf(stderr, "Recursor: to make a newsfeed, create the file <%s>.\n", news);
	}
	/* create Parsers attached to them */
	if(r->indexString    && !(r->indexParser    = Parser(r->indexString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", index);
	}
	if(r->sitemapString  && !(r->sitemapParser  = Parser(r->sitemapString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", map);		
	}
	if(r->newsfeedString && !(r->newsfeedParser = Parser(r->newsfeedString))) {
		fprintf(stderr, "Recursor: error generating Parser from <%s>.\n", news);
	}
	/* if theirs no content, we have nothing to do */
	if(!r->indexParser && !r->sitemapParser && !r->newsfeedParser) {
		fprintf(stderr, "Recursor: no Parsers defined, it would be useless to continue.\n");
		Recursor_();
		return 0;
	}
	/* parse the "header," ie, everything up to ~, the second arg is null
	 because we haven't set up the Files, so @files{}, @pwd{}, etc are undefined */
	ParserParse(r->sitemapParser,  0, 0, r->sitemap);
	ParserParse(r->newsfeedParser, 0, 0, r->newsfeed);
	return r;
}
void Recursor_(void) {
	if(!r) return;
	if(r->sitemapParser  && r->sitemap)  {
		ParserParse(r->sitemapParser,  0, -1, r->sitemap);
		ParserParse(r->sitemapParser,  0, 0,  r->sitemap);
	}
	if(r->sitemap  && fclose(r->sitemap))  perror(xmlSitemap);
	if(r->newsfeedParser && r->newsfeed) {
		ParserParse(r->newsfeedParser, 0, -1, r->newsfeed);
		ParserParse(r->newsfeedParser, 0, 0,  r->newsfeed);
	}
	if(r->newsfeed && fclose(r->newsfeed)) perror(rssNewsfeed);
	Parser_(r->indexParser);
	free(r->indexString);
	Parser_(r->sitemapParser);
	free(r->sitemapString);
	Parser_(r->newsfeedParser);
	free(r->newsfeedString);	
	free(r);
	r = 0;
}

/* entry-point (shouldn't have a prototype) */
int main(int argc, char **argv) {
	int ret;

	/* set up; fixme: dangerous! use stdarg, have a -delete, -write, and -help */
	if(argc <= 1) { usage(argv[0]); return EXIT_SUCCESS; }
	fprintf(stderr, "Changing directory to <%s>.\n", argv[1]);
	if(chdir(argv[1])) { perror(argv[1]); return EXIT_FAILURE; }

	/* make sure that umask is set so that others can read what we create */
	umask(S_IWGRP | S_IWOTH);

	/* recursing; fixme: this should be configurable */
	if(!Recursor(tmplIndex, tmplSitemap, tmplNewsfeed)) return EXIT_FAILURE;
	ret = recurse(0);
	Recursor_();
	return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* private */

int filter(const struct Files *files, const char *fn) {
	char *str, filed[64];
	FILE *fd;
	if(!r) { fprintf(stderr, "Recusor::filter: recursor not initialised.\n"); return 0; }
	/* *.d[.0]* */
	for(str = (char *)fn; (str = strstr(str, desc)); ) {
		str += strlen(desc);
		if(*str == '\0' || *str == '.') return 0;
	}
	/* *.news$ */
	if((str = strstr(fn, news))) {
		str += strlen(news);
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
	if(!strcmp(fn, dirCurrent)) return 0;
	/* .. */
	if(!strcmp(fn, dirParent) && FilesIsRoot(files)) return 0;
	/* index.html */
	if(!strcmp(fn, htmlIndex))  return 0;
	/* add .d, check 1 line for \n (hmm, this must be a real time waster) */
	if(strlen(fn) > sizeof(filed) - strlen(desc) - 1) {
		fprintf(stderr, "Recusor::filter: regected '%s' because it was too long (%d.)\n", fn, (int)sizeof(filed));
		return 0;
	}
	strcpy(filed, fn);
	strcat(filed, desc);
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
int recurse(const struct Files *parent) {
	struct Files *f;
	char         *name;
	FILE         *fp;
	f = Files(parent, &filter);
	/* write the index */
	if((fp = fopen(htmlIndex, "w"))) {
		ParserParse(r->indexParser, f, 0, fp);
		ParserRewind(r->indexParser);
		fclose(fp);
	} else perror(htmlIndex);
	/* sitemap */
	ParserParse(r->sitemapParser, f, 0, r->sitemap);
	ParserRewind(r->sitemapParser);
	/* recurse */
	while(FilesAdvance(f)) {
		if(!FilesIsDir(f) ||
		   !(name = FilesName(f)) ||
		   !strcmp(dirCurrent, name) ||
		   !strcmp(dirParent,  name) ||
		   !(name = FilesName(f))) continue;
		if(chdir(name)) { perror(name); continue; }
		recurse(f);
		/* this happens on Windows; I don't know what to do */
		if(chdir(dirParent)) perror(dirParent);
	}
	Files_(f);
	return -1;
}
char *readFile(const char *filename) {
	char *buf = 0, *newBuf;
	int bufPos = 0, bufSize = 0, read;
	FILE *fp;
	if(!filename) return 0;
	if(!(fp = fopen(filename, "r"))) { perror(filename); return 0; }
	for( ; ; ) {
		newBuf = realloc(buf, (bufSize += granularity) * sizeof(char));
		if(!newBuf) { perror(filename); free(buf); return 0; }
		buf = newBuf;
		read   =  fread(buf + bufPos, sizeof(char), granularity, fp);
		bufPos += read;
		if(read < granularity) { buf[bufPos] = '\0'; break; }
	}
	fprintf(stderr, "Opened '%s' and alloted %d bytes to read %d characters.\n", filename, bufSize, bufPos);
	if(fclose(fp)) perror(filename);
	return buf; /** you must free() the memory! */
}
void usage(const char *programme) {
	fprintf(stderr, "Usage: %s <directory>\n\n", programme);
	fprintf(stderr, "Version %d.%d.\n\n", versionMajor, versionMinor);
	fprintf(stderr, "MakeIndex is a content generator that places a changing\n");
	fprintf(stderr, "index.html on all the directories under <directory>\n");
	fprintf(stderr, "based on a template file in <directory> called <%s>.\n", tmplIndex);
	fprintf(stderr, "It also does some other stuff.\n\n");
	fprintf(stderr, "See readme.txt or http://neil.chaosnet.org/ for further info.\n\n");
	fprintf(stderr, "MakeIndex Copyright 2008, 2012 Neil Edelman\n");
	fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY.\n");
	fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
	fprintf(stderr, "under certain conditions; see gpl.txt.\n\n");
}
