struct Parser;
struct Files;

struct Parser *Parser(const char *str);
void Parser_(struct Parser *p);
void ParserRewind(struct Parser *p);
int ParserParse(struct Parser *p, const struct Files *f, int invisible, FILE *fp);
