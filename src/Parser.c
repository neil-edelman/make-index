/** Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt

 Parsing of strings. '~' on a line by itself is recognised in ".newsfile.rss"
 and ".sitemap.xml" as a <head>~<body>~<tail>; there should be two of them.

 Parsed in ".index.html",

 * @(content) prints {content.d}, or, if it is missing, {index.d};
 * @(files)\{} repeats for all the files in the directory the contents of the
   argument;
 * @(htmlcontent);
 * @(pwd)\{} prints the directory, the argument is the separator;
   eg, @(pwd)\{/};
 * @(root)\{}, like @(pwd), except up instead of down;
 * @(now) prints the date and the time in UTC.

 Further parsed in @(files){} in ".index.html",

 * @(filealt) prints Dir or File;
 * @(filedesc);
 * @(filehref) prints the filename or the {.link};
 * @(fileicon) prints it's {.d.jpeg}, or if it is not there, {dir.jpeg} or
   {file.jpeg};
 * @(filename) prints the file name;
 * @(filesize) prints the file size, if it exits;
 * @(now) prints the date and the time in UTC.

 Parsed in ".newsfeed.rss",

 * @(date) prints the date on the news file;
 * @(news) prints the contents of the file (as a text file);
 * @(newsname) prints the filename of the news story;
 * @(now) prints the date and the time in UTC;
 * @(title) prints the second line in the {.news}.

 @title		Parser
 @author	Neil
 @std		C89/90
 @version	1.1; 2017-03 fixed pedantic warnings; command-line improvements
 @since		0.6; 2008-03-21 */

#include <stdio.h>  /* [f]printf FILE */
#include <stdlib.h> /* malloc */
#include <string.h> /* strstr, strpbrk */
#include "Widget.h"
#include "Parser.h"

/* private */
static const int maxRecursion = 16;

/* public */
struct Parser {
	char *str;
	char *pos;
	char *rew;
	int  recursion;
};
/* private - this is the list of 'widgets', see Widget.c - add widgets to here
 to make them recognised - ASCIIbetical */
static const struct Symbol {
	const char *const symbol;
	const ParserWidget handler;
	const int onlyInRoot; /* fixme: not used, just trust the users? haha */
} sym[] = {
	{ "content",  &WidgetContent,  0 },  /* index */
	{ "date",     &WidgetDate,     0 },  /* news */
	{ "filealt",  &WidgetFilealt,  0 },  /* files */
	{ "filedesc", &WidgetFiledesc, 0 },  /* files */
	{ "filehref", &WidgetFilehref, 0 },  /* files */
	{ "fileicon", &WidgetFileicon, 0 },  /* files */
	{ "filename", &WidgetFilename, 0 },  /* files */
	{ "files",    &WidgetFiles,    -1 }, /* index */
	{ "filesize", &WidgetFilesize, 0 },  /* files */
	/*{ "folder",   0,               -1 }, *//* replaced by ~ - scetchy */
	{ "htmlcontent",&WidgetContent,0 },  /* index */
	{ "news",     &WidgetNews,     0 },  /* news */
	{ "newsname", &WidgetNewsname, 0 },  /* news */
	{ "now",      &WidgetNow,      0 },  /* any */
	{ "pwd",      &WidgetPwd,      -1 }, /* index */
	{ "root",     &WidgetRoot,     -1 }, /* like pwd except up instead of dn */
	{ "title",    &WidgetTitle,    0 }   /* news */
};

/* private */
static const struct Symbol *match(const char *str, const char *end);

/** @return Creates a parser for the string, {str}. */
struct Parser *Parser(char *const str) {
	struct Parser *p;
	if(!(p = malloc(sizeof *p))) return 0;
	p->str       = str;
	p->pos       = p->str;
	p->rew       = p->str;
	p->recursion = 0;
	return p;
}

/** @param p_ptr: A pointer to the {Parser} that's to be destucted. */
void Parser_(struct Parser **const p_ptr) {
	struct Parser *p;
	if(!p_ptr || !(p = *p_ptr)) return;
	if(p->recursion) fprintf(stderr, "Parser~: a file was closed with recursion level of %d; syntax error?\n", p->recursion);
	free(p);
	*p_ptr = 0;
}

/** Resets the parser, {p}. */
void ParserRewind(struct Parser *p) {
	if(!p) return;
	p->pos = p->rew;
}

/** Parse, called recursively.
 @param f: Called in the handler to {ParserWidget}s.
 @param invisible: No output.
 @param fp: Output.
 @fixme This fn needs rewriting; messy.
 @fixme Invisible, hack. */
int ParserParse(struct Parser *p, struct Files *const f, int invisible,
	FILE *fp) {
	char *mark;
	if(!p || !fp || !p->pos) return 0;
	if(++p->recursion > maxRecursion) fprintf(stderr, "Parser::parse: %d "
		"recursion levels reached (Ctrl-C to stop!)\n", p->recursion);
	mark = p->pos;
	if(p->recursion == 1) p->rew = mark;
	for( ; ; ) {
		p->pos = strpbrk(p->pos, "@}~");
		if(!p->pos) {
			if(!invisible) fprintf(fp, "%.*s", (int)(p->pos - mark), mark);
			break;
		} else if(*p->pos == '}') {
			if(!invisible) fprintf(fp, "%.*s", (int)(p->pos - mark), mark);
			p->pos++;
			break;
		} else if(*p->pos == '~') {
			/* a tilde on a line by itself */
			if(p->recursion == 1 &&
			   ((p->pos <= p->str || *(p->pos - 1) == '\n') &&
			   *(p->pos + 1) == '\n')) {
				if(!invisible) fprintf(fp, "%.*s", (int)(p->pos - mark), mark);
				p->pos += 2; /* "~\n" */
				p->recursion = 0;
				return -1;
			}
			p->pos++;
		} else if(*(p->pos + 1) == '(') { /* the only one left is @ */
			const struct Symbol *m;
			int                 over = 0, open;
			char                *start = p->pos + 2, *end;
			if(!invisible) fprintf(fp, "%.*s", (int)(p->pos - mark), mark);
			if(!(end = strpbrk(start, ")"))) break; /* syntax error */
			if(!(m = match(start, end))) fprintf(stderr, "Parser::parse: "
				"symbol not reconised, '%.*s.'\n", (int)(end - start), start);
			/* if(p->recursion == 1 && m && m->onlyInRoot) return -1; ? */
			open = *(end + 1) == '{' ? -1 : 0;
			do {
				/* -> widget.c */
				if(m && m->handler && !invisible) over = m->handler(f, fp);
				p->pos = end + (open ? 2 : 1);
				/* recurse between {} */
				if(open) ParserParse(p, f, invisible || !over, fp);
			} while(over);
			mark = p->pos;
		} else { /* @ by itself */
			p->pos++;
		}
	}
	p->recursion--;
	return 0;
}

/** binary search */
static const struct Symbol *match(const char *str, const char *end) {
	const int N = sizeof(sym) / sizeof(struct Symbol);
	int a, lo = 0, mid, hi = N - 1;
	size_t lenMatch, lenComp;
	lenMatch = end - str;
	while(lo <= hi) {
		mid = (lo + hi) >> 1;
		/* this is highly inefficient */
		lenComp = strlen(sym[mid].symbol);
		a = strncmp(str, sym[mid].symbol,
			lenMatch > lenComp ? lenMatch : lenComp);
		if     (a < 0) hi = mid - 1;
		else if(a > 0) lo = mid + 1;
		else           return &sym[mid];
	}
	return 0;
}
