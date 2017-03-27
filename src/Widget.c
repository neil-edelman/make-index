/* Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt

 Widgets like @files @pwd. To create a widget:

 * stick the code below implementing {ParserWidget};
 * create a prototype in Widget.h;
 * in {Parser.c}, add to the symbol table, sym[] with the symbol you want, in
   ASCIIbetical order.

 @title		Widget
 @author	Neil
 @std		C89/90
 @version	1.1; 2017-03 fixed pedantic warnings; command-line improvements
 @since		0.6; 2008-03-25 */

#include <string.h> /* strncat strncpy */
#include <stdio.h>  /* fprintf FILE */
#include <time.h>   /* time gmtime - for @date */
#include "Files.h"
#include "Parser.h"
#include "Recursor.h"
#include "Widget.h"

/* <-- ugly */
#ifndef _MSC_VER /* <-- not msvc */
#define UNUSED(a) while(0 && (a))
#else /* not msvc --><-- msvc: not a C89/90 compiler; needs a little help */
#pragma warning(push)
/* "Assignment within conditional expression." No. */
#pragma warning(disable: 4706)
/* "<ANSI/ISO name>: The POSIX name for this item is deprecated." No. */
#pragma warning(disable: 4996)
#define UNUSED(a) (void)(sizeof((a), 0))
#endif /* msvc --> */
/* ugly --> */

/* constants */
static const char *html_desc    = "index.d";
static const char *html_content = "content.d";
static const char *separator    = "/";
static const char *picture      = ".jpeg"; /* yeah, I hard coded this */
static const size_t max_read    = 512;
static const char *dot_link     = ".link";
const char *dot_desc            = ".d"; /* used in multiple files */
const char *dot_news            = ".news";
extern const char *dir_current;
extern const char *dir_parent;
extern const char *html_index;

/* global, ick: news */
static int year          = 1983;
static int month         = 1;
static int day           = 30;
static char title[64]    = "(no title)";
static char filenews[64] = "(no file name)";

/* private */
int clip(int no, const int low, const int high);

int WidgetSetNews(const char *fn) {
	char *dot;
	int  read;
	size_t tLen;
	FILE *fp;
	if(!fn || !(dot = strstr(fn, dot_news))) return 0;
	if(strlen(fn) > sizeof(filenews) / sizeof(char) - sizeof(char)) {
		fprintf(stderr, "Widget::SetNews: news file name, \"%s,\" doesn't fit "
			"in buffer (%d.)\n", fn, (int)(sizeof(filenews) / sizeof(char)));
		return 0;
	}
	/* save the fn, safe because we checked it, and strip off .news */
	strcpy(filenews, fn);
	filenews[dot - fn] = '\0';
	/* open .news */
	if(!(fp = fopen(fn, "r"))) { perror(fn); return 0; }
	read = fscanf(fp, "%d-%d-%d\n", &year, &month, &day);
	if(read < 3) fprintf(stderr, "Widget::setNews: error parsing ISO 8601 "
		"date, <YYYY-MM-DD>.\n");
	month = clip(month, 1, 12);
	day   = clip(day,   1, 31);
	/* fgets reads a newline at the end (annoying) so we strip that off */
	if(!fgets(title, (int)sizeof(title), fp)) { perror(fn); *title = '\0'; }
	else if((tLen = strlen(title)) > 0 && title[tLen - 1] == '\n')
		title[tLen - 1] = '\0';
	if(fclose(fp)) perror(fn);
	fprintf(stderr, "News <%s>, '%s' %d-%d-%d.\n", filenews, title, year,
		month, day);
	return -1;
}

/* the widget handlers */

/** @implements ParserWidget */
int WidgetContent(struct Files *const f, FILE *const fp) {
	char buf[81], *bufpos;
	size_t i;
	FILE *in;

	UNUSED(f);
	/* it's a nightmare to test if this is text (which most is,) in which case
	 we should insert <p>...</p> after every paragraph, <>& -> &lt;&gt;&amp;,
	 but we have to not translate already encoded html; the only solution that
	 a could see is have a new langauge (like-LaTeX) that gracefully handles
	 plain-text */
	if((in = fopen(html_content, "r")) || (in = fopen(html_desc, "r"))) {
		for(i = 0; (i < max_read)
			&& (bufpos = fgets(buf, (int)sizeof(buf), in)); i++) {
			fprintf(fp, "%s", bufpos);
		}
		if(fclose(in)) perror(html_desc);
	}
	return 0;
}
/** @implements ParserWidget */
int WidgetDate(struct Files *const f, FILE *const fp) {
	UNUSED(f);
	/* ISO 8601 - YYYY-MM-DD */
	fprintf(fp, "%4.4d-%2.2d-%2.2d", year, month, day);
	return 0;
}
/** @implements ParserWidget */
int WidgetFilealt(struct Files *const f, FILE *const fp) {
	fprintf(fp, "%s", FilesIsDir(f) ? "Dir" : "File");
	return 0;
}
/** @implements ParserWidget */
int WidgetFiledesc(struct Files *const f, FILE *const fp) {
	char buf[256];
	const char *name;
	FILE *in;
	if(!(name = FilesName(f))) return 0;
	if(FilesIsDir(f)) {
		/* <file>/index.d */
		strncpy(buf, name, sizeof(buf) - 9);
		strncat(buf, separator, 1lu);
		strncat(buf, html_desc, 7lu);
	} else {
		/* <file>.d */
		strncpy(buf, name, sizeof(buf) - 6);
		strncat(buf, dot_desc, 5lu);
	}
	if((in = fopen(buf, "r"))) {
		char *bufpos;
		size_t i;
		for(i = 0; (i < max_read)
			&& (bufpos = fgets(buf, (int)sizeof(buf), in)); i++) {
			fprintf(fp, "%s", bufpos);
		}
		if(fclose(in)) perror(buf);
	}
	return 0;
}
/** @implements ParserWidget */
int WidgetFilehref(struct Files *const f, FILE *const fp) {
	const char *str, *name;
	int ch;
	FILE *fhref;
	if(!(name = FilesName(f))) return 0;
	if((str = strstr(name, dot_link)) && *(str += strlen(dot_link)) == '\0'
		&& (fhref = fopen(name, "r"))) {
		/* fixme: not too good, with the reading one char at a time */
		for( ; ; ) {
			ch = fgetc(fhref);
			if(ch == '\n' || ch == '\r' || ch == EOF) break;
			fprintf(fp, "%c", ch);
		}
		if(fclose(fhref)) perror(name);
	} else {
		fprintf(fp, "%s", name);
	}
	return 0;
}
/** @implements ParserWidget */
int WidgetFileicon(struct Files *const f, FILE *const fp) {
	char buf[256];
	const char *name;
	FILE *in;
	if(!(name = FilesName(f))) return 0;
	/* insert <file>.d.jpeg if available */
	strncpy(buf, name,    sizeof(buf) - 12);
	strncat(buf, dot_desc,5lu);
	strncat(buf, picture, 6lu);
	if((in = fopen(buf, "r"))) {
		fprintf(fp, "%s", buf);
		if(fclose(in)) perror(buf);
	} else {
		/* added thing to get to root instead of / because sometimes 'root'
		 is not the real root! eg www.geocities.com/~foo/; does the same thing
		 as having a @root{/} */
		FilesSetPath((struct Files *)f);
		while(FilesEnumPath((struct Files *)f)) {
			fprintf(fp, "%s%s", dir_parent, separator);
		}
		fprintf(fp, "%s%s", FilesIsDir(f) ? "dir" : "file", picture);
	}
	return 0;
}
/** @implements ParserWidget */
int WidgetFilename(struct Files *const f, FILE *const fp) {
	fprintf(fp, "%s", FilesName(f));
	return 0;
}
/** @implements ParserWidget */
int WidgetFiles(struct Files *const f, FILE *const fp) {
	while(0 && fp);
	return FilesAdvance((struct Files *)f) ? -1 : 0;
}
/** @implements ParserWidget */
int WidgetFilesize(struct Files *const f, FILE *const fp) { /* eww */
	if(!FilesIsDir(f)) fprintf(fp, " (%d KB)", FilesSize(f));
	return 0;
}
/** @implements ParserWidget */
int WidgetNews(struct Files *const f, FILE *const fp) {
	char buf[256], *bufpos;
	size_t i;
	FILE *in;

	UNUSED(f);
	if(!filenews[0]) return 0;
	if(!(in = fopen(filenews, "r"))) { perror(filenews); return 0; }
	for(i = 0; (i < max_read) && (bufpos = fgets(buf, (int)sizeof(buf), in));
		i++){
		fprintf(fp, "%s", bufpos);
	}
	if(fclose(in)) perror(filenews);
	return 0;
}
/** @implements ParserWidget */
int WidgetNewsname(struct Files *const f, FILE *const fp) {
	UNUSED(f);
	fprintf(fp, "%s", filenews);
	return 0;
}
/** @implements ParserWidget */
int WidgetNow(struct Files *const f, FILE *const fp) {
	char      t[22];
	time_t    currentTime;
	struct tm *formatedTime;

	UNUSED(f);
	if((currentTime = time(0)) == (time_t)(-1)) { perror("@date"); return 0; }
	formatedTime = gmtime(&currentTime);
	/* ISO 8601 - YYYY-MM-DDThh:mm:ssTZD */
	strftime(t, 21lu, "%Y-%m-%dT%H:%M:%SZ", formatedTime);
	fprintf(fp, "%s", t);
	return 0;
}
/** @implements ParserWidget */
int WidgetPwd(struct Files *const f, FILE *const fp) {
	static int persistant = 0; /* ick, be very careful! */
	const char *pwd;
	if(!persistant) { persistant = -1; FilesSetPath((struct Files *)f); }
	pwd = FilesEnumPath(f);
	if(!pwd)        { persistant = 0;  return 0; }
	fprintf(fp, "%s", pwd);
	return -1;
}
/** @implements ParserWidget */
int WidgetRoot(struct Files *const f, FILE *const fp) {
	static int persistant = 0; /* ick, be very careful! */
	const char *pwd;
	if(!persistant) { persistant = -1; FilesSetPath((struct Files *)f); }
	pwd = FilesEnumPath(f);
	if(!pwd)        { persistant = 0;  return 0; }
	fprintf(fp, "%s", dir_parent);
	return -1;
}
/** @implements ParserWidget */
int WidgetTitle(struct Files *const f, FILE *const fp) {
	UNUSED(f);
	fprintf(fp, "%s", title);
	return 0;
}

int clip(int no, const int low, const int high) {
	if(no < low)       no = low;
	else if(no > high) no = high;
	return no;
}
