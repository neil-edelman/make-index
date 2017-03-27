/** See \see{Parser}. */
struct Parser;

/** Dependancy on {Files}. */
struct Files;

/** All {ParserWidget}s are in {Widget.c}. */
typedef int (*ParserWidget)(struct Files *const files, FILE *const fp);

struct Parser *Parser(char *const str);
void Parser_(struct Parser **const p_ptr);
void ParserRewind(struct Parser *p);
int ParserParse(struct Parser *p, struct Files *const f, int invisible,
	FILE *fp);
