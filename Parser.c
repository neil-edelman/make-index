/* Copyright 2008, 2012 Neil Edelman, distributed under the terms of the
 GNU General Public License, see copying.txt */

/* Parsing of strings.
 * Created by Neil Edelman on 2008-03-21. */

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
struct Symbol {
	char *symbol;
	int (*handler)(const struct Files *, FILE *fp);
	int onlyInRoot; /* fixme: not used, just trust the users? haha */
} static const sym[] = {
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
	{ "pwd",      &WidgetPwd,      -1 }, /* don't put it where it doesn't make sense */
	{ "root",     &WidgetRoot,     -1 }, /* like pwd exept up instead of dn */
	{ "title",    &WidgetTitle,    0 }   /* news */
};

/* private */
int parse(struct Parser *p, int invisible, FILE *fp);

struct Parser *Parser(const char *str) {
	struct Parser *p;
	p = malloc(sizeof(struct Parser));
	p->str       = (char *)str;
	p->pos       = p->str;
	p->rew       = p->str;
	p->recursion = 0;
	return p;
}
void Parser_(struct Parser *p) {
	if(!p) return;
	if(p->recursion) fprintf(stderr, "Parser~: a file was closed with recursion level of %d; syntax error?\n", p->recursion);
	free(p);
}

/** binary search */
const struct Symbol *match(const char *str, const char *end) {
	const int N = sizeof(sym) / sizeof(struct Symbol);
	int a, lenMatch, lenComp, lo = 0, mid, hi = N - 1;
	lenMatch = end - str;
	while(lo <= hi) {
		mid = (lo + hi) >> 1;
		/* this is highly inefficient */
		lenComp = strlen(sym[mid].symbol);
		a = strncmp(str, sym[mid].symbol, lenMatch > lenComp ? lenMatch : lenComp);
		if     (a < 0) hi = mid - 1;
		else if(a > 0) lo = mid + 1;
		else           return &sym[mid];
	}
	return 0;
}
void ParserRewind(struct Parser *p) {
	if(!p) return;
	p->pos = p->rew;
}
/** parse, called recusively (invisible, hack) fixme: this fn needs rewriting, messy */
int ParserParse(struct Parser *p, const struct Files *f, int invisible, FILE *fp) {
	char *mark;
	if(!p || !fp || !p->pos) return 0;
	if(++p->recursion > maxRecursion) fprintf(stderr, "Parser::parse: %d recursion levels reached (Ctrl-C to stop!)\n", p->recursion);
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
			if(!(m = match(start, end))) fprintf(stderr, "Parser::parse: symbol not reconised, '%.*s.'\n", (int)(end - start), start);
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
