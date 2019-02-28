/** 2019 Neil Edelman, distributed under the terms of the MIT License;
 see readme.txt, or \url{ https://opensource.org/licenses/MIT }.
 
 \url{ https://www.codeproject.com/Articles/1035799/Generating-a-High-Speed-Parser-Part-re-c }

 */

#include <stdlib.h> /* EXIT fopen */
#include <stdio.h>  /* fprintf */
#include <assert.h> /* assert */
#include <errno.h>

static const char *const default_input_fn = "a.void";
static const char *const default_gv_fn = "a.gv";

/*!re2c
	re2c:yyfill:enable = 0;
	re2c:define:YYCTYPE = char;
	re2c:define:YYCURSOR = s->cursor;
	re2c:define:YYMARKER = s->marker;
*/

/* re2c:flags:utf-8 = 1; crashes? */

struct Scanner {
	const char *top, *cursor, *marker, *position, *end;
	size_t line;
};

/* X-Marco. */
#define IDENT(A) A
#define STRINGISE(A) #A
#define TOKEN(X) \
	X(UNKNOWN), X(START), X(LBRACE), X(RBRACE), X(LBRACK), X(RBRACK), \
	X(COLON), X(COMMA), X(SEMI), X(NAME), X(STRING), X(INT), X(SIGNED), \
	X(FLOAT), X(TYPE), X(TYPE_AUTO), X(TYPE_KEY), X(TYPE_STRING), X(TYPE_INT), \
	X(TYPE_SIGNED), X(TYPE_FLOAT), X(TYPE_VEC2I), X(TYPE_VEC3I), \
	X(TYPE_ORTHO3I), X(END)
enum Token { TOKEN(IDENT) };
static const char *const tokens[] = { TOKEN(STRINGISE) };

/** Initialise {s} with {buffer} and {size}. */
static void Scanner(struct Scanner *const s,
	const char *const buffer, const size_t size) {
	assert(s && buffer && size);
	s->top = s->cursor = s->position = s->marker = buffer;
	s->end = buffer + size;
	s->line = 1;
}

static enum Token scan(struct Scanner *const s) {
	assert(s);

regular:
	if(s->cursor >= s->end) return END;
	s->top = s->cursor;
/*!re2c
	whitespace = [ \t\v\f]+;
	notwhite = [^ \t\n\v\f\r\x00];
	newline = "\r\n" | "\n" | "\r";
	digit = [0-9];
	id = [a-zA-Z_];
	string = ("\"" [^"]* "\"") | ("\'" [^']* "\'") | ("`" [^`]* "`")
		| ("<" [^>]* ">");

	"/*" { goto comment; }
	"{" { return LBRACE; }
	"}" { return RBRACE; }
	"," { return COMMA; }
	";" { return SEMI; }
	"[" { return LBRACK; }
	"]" { return RBRACK; }
	":" { return COLON; }
	"type" { return TYPE; }
	"auto" { return TYPE_AUTO; }
	"key" { return TYPE_KEY; }
	"string" { return TYPE_STRING; }
	"int" { return TYPE_INT; }
	"signed" { return TYPE_SIGNED; }
	"float" { return TYPE_FLOAT; }
	"vec2i" { return TYPE_VEC2I; }
	"vec3i" { return TYPE_VEC3I; }
	"ortho3i" { return TYPE_ORTHO3I; }
	id (id | digit)* { return NAME; }
	string { return STRING; }
	"-"digit+ { return SIGNED; }
	digit+ { return INT; }
	whitespace { goto regular; }
	newline { s->position = s->cursor; s->line++; goto regular; }
	* { return UNKNOWN; }
*/

comment:
	if(s->cursor >= s->end) return UNKNOWN;
/*!re2c
	"*/" { goto regular; }
	newline { s->line++; goto comment; }
	* { goto comment; }
*/
}

struct Node {
	enum Token token;
};

static void node_to_string(const struct Node *v, char (*const a)[12]) {
	sprintf(*a, "%.11s", tokens[v->token]);
}

#define DIGRAPH_NAME Parse
#define DIGRAPH_VDATA struct Node
#define DIGRAPH_VDATA_TO_STRING &node_to_string
#define DIGRAPH_FLOW
#include "../src/Digraph.h"

static void vertex_migrate(struct ParseVertexLink *const v,
	const struct Migrate *const migrate) {
	assert(v && migrate);
	ParseVertexLinkMigrate(&v->data, migrate);
	ParseEdgeListSelfCorrect(&v->data.out);
}

#define POOL_NAME Vertex
#define POOL_TYPE struct ParseVertexLink
#define POOL_MIGRATE_EACH &vertex_migrate
#define POOL_MIGRATE_ALL struct ParseDigraph
#include "../src/Pool.h"

static void edge_migrate(struct ParseEdgeLink *const e,
	const struct Migrate *const migrate) {
	assert(e && migrate);
	ParseEdgeLinkMigrate(&e->data, migrate);
}

#define POOL_NAME Edge
#define POOL_TYPE struct ParseEdgeLink
#define POOL_MIGRATE_EACH &edge_migrate
#include "../src/Pool.h"

struct Parse {
	struct ParseDigraph graph;
	struct VertexPool vs;
	struct EdgePool es;
};

static void Parse_(struct Parse *const p) {
	if(!p) return;
	ParseDigraph_(&p->graph);
	VertexPool_(&p->vs);
	EdgePool_(&p->es);
}

static void Parse(struct Parse *const p) {
	assert(p);
	ParseDigraph(&p->graph);
	VertexPool(&p->vs, &ParseDigraphVertexMigrateAll, &p->graph);
	EdgePool(&p->es);
}

int main(int argc, char **argv) {
	struct Parse p;
	struct Scanner s;
	FILE *fp = 0;
	char *contents = 0;
	/* @fixme: GNU {getops} is much better. */
	const char *const in_fn = (argc > 1) ? argv[1] : default_input_fn,
		*const gv_fn = (argc > 2) ? argv[2] : default_gv_fn;
	int is_data = 0, is_graph = 0, is_parse = 0;
	const char *err = 0;

	do { /* Try: read {contents}. */

		/* Read the entire file as a big binary blob. */
		size_t size, read;
		long signed_size;
		if(!(fp = fopen(in_fn, "r")) || fseek(fp, 0l, SEEK_END) == -1
			|| (signed_size = ftell(fp)) == -1
			|| fseek(fp, 0l, SEEK_SET) == -1) break;
		if(signed_size <= 0)
			{ fprintf(stderr, "Empty file.\n"); errno = EDOM; break; }
		size = signed_size;
		if(!(contents = malloc(size))
			|| (read = fread(contents, 1, size, fp), read != size)) break;
		/* Debug.
		fwrite(contents, size, 1, stdout); */

		/* Set up the scanner. */
		Scanner(&s, contents, size);

		is_data = 1;
	} while(0); if(!is_data) {
		perror(in_fn);
	} {
		if(fp) fclose(fp), fp = 0;
	}

	Parse(&p);
	do { /* Try: build a DFA. */
		struct ParseVertexLink *vs[7];
		const size_t vs_size = sizeof vs / sizeof *vs;
		struct ParseEdgeLink *es[7];
		const size_t es_size = sizeof es / sizeof *es;
		size_t i;
		if(!is_data) { is_graph = 1; break; } /* Error from before. */

		/* Allocate the starting pointers. */
		for(i = 0; i < vs_size; i++) if(!(vs[i] = VertexPoolNew(&p.vs))) break;
		if(i != vs_size) break;
		for(i = 0; i < es_size; i++) if(!(es[i] = EdgePoolNew(&p.es))) break;
		if(i != es_size) break;

		/* Connect them. */
		ParseDigraphVertexData(&vs[0]->data)->token = START;
		ParseDigraphVertexData(&vs[1]->data)->token = END;

		ParseDigraphVertexData(&vs[2]->data)->token = TYPE;
		ParseDigraphVertexData(&vs[3]->data)->token = NAME;
		ParseDigraphVertexData(&vs[4]->data)->token = LBRACE;
		ParseDigraphVertexData(&vs[5]->data)->token = RBRACE;
		ParseDigraphVertexData(&vs[6]->data)->token = SEMI;

		for(i = 0; i < vs_size; i++)
			ParseDigraphPutVertex(&p.graph, &vs[i]->data);
		ParseDigraphSetSink(&p.graph, &vs[1]->data);

		/* Connection start->end. */
		ParseDigraphPutEdge(&es[0]->data, &vs[0]->data, &vs[1]->data);

		/* Connection start->type{}; */
		ParseDigraphPutEdge(&es[1]->data, &vs[0]->data, &vs[2]->data);
		ParseDigraphPutEdge(&es[2]->data, &vs[2]->data, &vs[3]->data);
		ParseDigraphPutEdge(&es[3]->data, &vs[3]->data, &vs[4]->data);
		ParseDigraphPutEdge(&es[4]->data, &vs[4]->data, &vs[5]->data);
		ParseDigraphPutEdge(&es[5]->data, &vs[5]->data, &vs[6]->data);
		ParseDigraphPutEdge(&es[6]->data, &vs[6]->data, &vs[0]->data);

		if(!(fp = fopen(gv_fn, "w"))) break;
		ParseDigraphOut(&p.graph, 0, fp);

		is_graph = 1;
	} while(0); if(!is_graph) {
		perror(gv_fn);
	} {
		if(fp) fclose(fp), fp = 0;
	}

	do { /* Try: parse. */
		enum Token t;
		struct ParseVertex *v = ParseDigraphGetSource(&p.graph);
		struct ParseEdge *e;
		if(!is_data || !is_graph) { is_parse = 1; break; }

		do {
			t = scan(&s);

			/* Advance the parsing. */
			for(e = ParseEdgeListFirst(&v->out); e; e = ParseEdgeListNext(e)) {
				if(t != ParseDigraphVertexData(e->to)->token) continue;
				v = e->to;
				break;
			}
			if(!e) {
				char a[12];
				printf("Found %s; expected:", tokens[t]);
				for(e = ParseEdgeListFirst(&v->out); e;
					e = ParseEdgeListNext(e)) {
						node_to_string(ParseDigraphVertexData(e->to), &a);
						printf(" %s", a);
				}
				printf(".\n");
				err = "syntax error";
				break;
			}

			switch(t) {
			case LBRACE: printf("<{>"); break;
			case RBRACE: printf("<}>"); break;
			case LBRACK: printf("<[>"); break;
			case RBRACK: printf("<]>"); break;
			case COLON: printf("<:>"); break;
			case COMMA: printf("<,>"); break;
			case SEMI: printf("<;>\n\n"); break;
			case NAME: printf("<name:%.*s>", (int)(s.cursor - s.top), s.top); break;
			case STRING: printf("<string:%.*s>", (int)(s.cursor - s.top), s.top);break;
			case INT: printf("<int:%.*s>", (int)(s.cursor - s.top), s.top); break;
			case SIGNED: printf("<signed:%.*s>", (int)(s.cursor - s.top), s.top);break;
			case FLOAT: printf("<float:%.*s>", (int)(s.cursor - s.top), s.top); break;
			case TYPE: printf("<type>"); break;
			case TYPE_AUTO: printf("<auto type>"); break;
			case TYPE_KEY: printf("<key type>"); break;
			case TYPE_STRING: printf("<string type>"); break;
			case TYPE_INT: printf("<int type>"); break;
			case TYPE_SIGNED: printf("<signed type>"); break;
			case TYPE_FLOAT: printf("<float type>"); break;
			case TYPE_VEC2I: printf("<vec2i type>"); break;
			case TYPE_VEC3I: printf("<vec3i type>"); break;
			case TYPE_ORTHO3I: printf("<ortho3i type>"); break;
			case END: printf("<end>\n\n"); break;
			case START: printf("<start?>"); break; /* Not possible. */
			case UNKNOWN: err = "unknown token"; break;
			}
		} while(t != END && t != UNKNOWN);
		printf("->%s\n", tokens[t]);
		if(t != END) break;
		/* This irritates me, so I made it an error. */
		if(s.end[-1] != '\n' && s.end[-1] != '\r')
			{ err = "no newline at EOF"; break; }

		is_parse = 1;
	} while(0); if(!is_parse) {
		/* @fixme: top - pos is wrong [at eof]. */
		fprintf(stderr, "%s:%lu:%lu: %s\n", in_fn, (unsigned long)s.line,
			(unsigned long)(s.top - s.position) + 1, err);
	} {
		free(contents), contents = 0;
		Parse_(&p);
	}
	return is_data && is_graph && is_parse ? EXIT_SUCCESS : EXIT_FAILURE;
}
