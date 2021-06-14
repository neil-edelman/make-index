extern const char *dot_desc, *dot_news;

struct Recursor;
struct Files;

void WidgetSetRecursor(const struct Recursor *recursor);
int WidgetWriteNews(const char *fn);
/* the widget handlers */
int WidgetDate(struct Files *const f, FILE *const fp);
int WidgetContent(struct Files *const f, FILE *const fp);
int WidgetFilealt(struct Files *const f, FILE *const fp);
int WidgetFiledesc(struct Files *const f, FILE *const fp);
int WidgetFilehref(struct Files *const f, FILE *const fp);
int WidgetFileicon(struct Files *const f, FILE *const fp);
int WidgetFilename(struct Files *const f, FILE *const fp);
int WidgetFiles(struct Files *const f, FILE *const fp);
int WidgetFilesize(struct Files *const f, FILE *const fp);
int WidgetNews(struct Files *const f, FILE *const fp);
int WidgetNewsname(struct Files *const f, FILE *const fp);
int WidgetNow(struct Files *const f, FILE *const fp);
int WidgetPwd(struct Files *const f, FILE *const fp);
int WidgetRoot(struct Files *const f, FILE *const fp);
int WidgetTitle(struct Files *const f, FILE *const fp);
