/** @license 2000, 2012 Neil Edelman, distributed under the terms of the
 [GNU General Public License 3](https://opensource.org/licenses/GPL-3.0).

 @subtitle Widget
 @author Neil

 Widgets like @(files), @(pwd). To create a widget:

 \* stick the code below implementing `ParserWidget`;
 \* create a prototype in `Widget.h`;
 \* in `Parser.c`, add to the symbol table, `sym[]` with the symbol you want,
    in ASCIIbetical order.

 See Parser for more information.

 @std POSIX.1 */

/* 2017-03 fixed pedantic warnings; command-line improvements
 2008-03-25 */

#include <string.h> /* strncat strncpy */
#include <stdio.h>  /* fprintf FILE */
#include <time.h>   /* time gmtime - for @date */
#include <errno.h>
#include <assert.h>
#include "Files.h"
#include "Parser.h"
#include "Widget.h"

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

/* global, ick: news */
static int year          = 1969;
static int month         = 7;
static int day           = 20;
static char title[64]    = "(no title)";
static char filenews[64] = "(no file name)";

/** @return `no` clipped between [`low`, `high`]. */
static int clip(int no, const int low, const int high) {
	assert(low <= high);
	if(no < low)       no = low;
	else if(no > high) no = high;
	return no;
}

/** Reads the news from `fn` into static variables. @return Success. */
int WidgetWriteNews(const char *fn) {
	char *dot;
	int  read;
	size_t tLen;
	FILE *fp;
	if(!fn || !(dot = strstr(fn, dot_news)) || strlen(fn) > sizeof filenews - 1)
		return fprintf(stderr,
		"Widget::WriteNews: news file invalid or too long (%lu,) <%s>.\n",
		(unsigned long)sizeof filenews, fn), errno = EDOM, 0;
	/* save the fn, safe because we checked it, and strip off .news */
	strcpy(filenews, fn);
	filenews[dot - fn] = '\0';
	/* open .news */
	if(!(fp = fopen(fn, "r"))) return 0;
	read = fscanf(fp, "%d-%d-%d\n", &year, &month, &day);
	if(read < 3) return fprintf(stderr,
		"Widget::WriteNews: error parsing ISO 8601, <YYYY-MM-DD>, <%s>.\n",
		fn), errno = EDOM, 0;
	month = clip(month, 1, 12);
	day   = clip(day,   1, 31);
	/* fgets reads a newline at the end (annoying) so we strip that off */
	if(!fgets(title, (int)sizeof(title), fp)) { perror(fn); *title = '\0'; }
	else if((tLen = strlen(title)) > 0 && title[tLen - 1] == '\n')
		title[tLen - 1] = '\0';
	if(fclose(fp)) perror(fn);
	fprintf(stderr, "News <%s>, '%s' %d-%d-%d.\n",
		filenews, title, year, month, day);
	return 1;
}

/* the widget handlers */

/** Displays the content, (either `index.d` or `content.d`.) Ignores `f` and
 writes to `fp`. @implements ParserWidget @return Success. */
int WidgetContent(struct Files *const f, FILE *const fp) {
	char buf[81], *bufpos;
	size_t i;
	FILE *in;
	(void)f;
	assert(fp);
	/* it's a nightmare to test if this is text (which most is,) in which case
	 we should insert <p>...</p> after every paragraph, <>& -> &lt;&gt;&amp;,
	 but we have to not translate already encoded html; the only solution that
	 I could see is have a new language (like-LaTeX) that gracefully handles
	 plain-text */
	if((in = fopen(html_content, "r")) || (in = fopen(html_desc, "r"))) {
		for(i = 0; (i < max_read)
			&& (bufpos = fgets(buf, (int)sizeof buf, in)); i++) {
			fprintf(fp, "%s", bufpos);
		}
		if(fclose(in) == EOF) perror("index");
	}
	return 0;
}
/** Ignores `f` and writes to `fp`. @implements ParserWidget */
int WidgetDate(struct Files *const f, FILE *const fp) {
	(void)f;
	/* ISO 8601 - YYYY-MM-DD */
	fprintf(fp, "%4.4d-%2.2d-%2.2d", year, month, day);
	return 0;
}
/** Writes to `fp` whether `f` is "Dir" or "File". @implements ParserWidget */
int WidgetFilealt(struct Files *const f, FILE *const fp) {
	fprintf(fp, "%s", FilesIsDir(f) ? "Dir" : "File");
	return 0;
}
/** Writes to `fp` the description of `f`, that is the `.d` file, if it can
 find it. @implements ParserWidget */
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
			&& (bufpos = fgets(buf, (int)sizeof(buf), in)); i++)
			fprintf(fp, "%s", bufpos);
		if(fclose(in)) perror(buf);
	}
	return 0;
}
/** Writes to `fp` the first line in `f`. @implements ParserWidget */
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
/** Writes to `fp` an icon of `f`, `.d.jpeg` if available.
 @implements ParserWidget */
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
/** Writes to `fp` the name of `f`. @implements ParserWidget */
int WidgetFilename(struct Files *const f, FILE *const fp) {
	fprintf(fp, "%s", FilesName(f));
	return 0;
}
/** Ignores `fp` and advances the global file from `f`.
 @implements ParserWidget */
int WidgetFiles(struct Files *const f, FILE *const fp) {
	(void)fp;
	return FilesAdvance((struct Files *)f) ? -1 : 0;
}
/** Writes to `fp` the size of `f` in KB. @implements ParserWidget */
int WidgetFilesize(struct Files *const f, FILE *const fp) {
	if(!FilesIsDir(f)) fprintf(fp, " (%d KB)", FilesSize(f));
	return 0;
}
/** Ignores `f`, writes to `fp` the news contained in a global.
 @implements ParserWidget */
int WidgetNews(struct Files *const f, FILE *const fp) {
	char buf[256], *bufpos;
	size_t i;
	FILE *in;
	(void)f;
	if(!filenews[0]) return 0;
	if(!(in = fopen(filenews, "r"))) { perror(filenews); return 0; }
	for(i = 0; (i < max_read) && (bufpos = fgets(buf, (int)sizeof(buf), in));
		i++) fprintf(fp, "%s", bufpos);
	if(fclose(in)) perror(filenews);
	return 0;
}
/** Ignores `f`. Writes to `fp` the global name of the current news.
 @implements ParserWidget */
int WidgetNewsname(struct Files *const f, FILE *const fp) {
	(void)f;
	fprintf(fp, "%s", filenews);
	return 0;
}
/** Ignores `f`. Writes to `fp` the date. @implements ParserWidget */
int WidgetNow(struct Files *const f, FILE *const fp) {
	char      t[22];
	time_t    currentTime;
	struct tm *formatedTime;
	(void)f;
	if((currentTime = time(0)) == (time_t)(-1)) { perror("@date"); return 0; }
	formatedTime = gmtime(&currentTime);
	/* ISO 8601 - YYYY-MM-DDThh:mm:ssTZD */
	strftime(t, 21lu, "%Y-%m-%dT%H:%M:%SZ", formatedTime);
	fprintf(fp, "%s", t);
	return 0;
}
/** Writes to `fp` the path of `f`. @implements ParserWidget */
int WidgetPwd(struct Files *const f, FILE *const fp) {
	static int persistant = 0; /* ick, be very careful! */
	const char *pwd;
	/* fixme: Const-correctness! You can't do that. */
	if(!persistant) { persistant = 1; FilesSetPath((struct Files *)f); }
	pwd = FilesEnumPath(f);
	if(!pwd)        { persistant = 0;  return 0; }
	fprintf(fp, "%s", pwd);
	return -1;
}
/** Writes to `fp` the path of `f` in reverse. @implements ParserWidget */
int WidgetRoot(struct Files *const f, FILE *const fp) {
	static int persistant = 0; /* ick, be very careful! */
	const char *pwd;
	if(!persistant) { persistant = 1; FilesSetPath((struct Files *)f); }
	pwd = FilesEnumPath(f);
	if(!pwd)        { persistant = 0;  return 0; }
	fprintf(fp, "%s", dir_parent);
	return -1;
}
/** Ignores `f`. Writes to `fp` the current global title.
 @implements ParserWidget */
int WidgetTitle(struct Files *const f, FILE *const fp) {
	(void)f;
	fprintf(fp, "%s", title);
	return 0;
}
