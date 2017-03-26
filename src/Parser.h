/** See \see{Parser}. */
struct Parser;

/** Dependancy on {Files} */
struct Files;

/** All {ParserWidget}s are in {Widget.c} */
typedef int (*ParserWidget)(const struct Files *, FILE *fp);

struct Parser *Parser(const char *str);
void Parser_(struct Parser **const p_ptr);
void ParserRewind(struct Parser *p);
int ParserParse(struct Parser *p, const struct Files *f, int invisible,
	FILE *fp);
