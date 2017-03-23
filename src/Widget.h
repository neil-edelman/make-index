struct Recursor;
struct Files;

void WidgetSetRecursor(const struct Recursor *recursor);
int WidgetSetNews(const char *fn);
/* the widget handlers */
int WidgetDate(const struct Files *f, FILE *fp);
int WidgetContent(const struct Files *f, FILE *fp);
int WidgetFilealt(const struct Files *f, FILE *fp);
int WidgetFiledesc(const struct Files *f, FILE *fp);
int WidgetFilehref(const struct Files *f, FILE *fp);
int WidgetFileicon(const struct Files *f, FILE *fp);
int WidgetFilename(const struct Files *f, FILE *fp);
int WidgetFiles(const struct Files *f, FILE *fp);
int WidgetFilesize(const struct Files *f, FILE *fp);
int WidgetNews(const struct Files *f, FILE *fp);
int WidgetNewsname(const struct Files *f, FILE *fp);
int WidgetNow(const struct Files *f, FILE *fp);
int WidgetPwd(const struct Files *f, FILE *fp);
int WidgetRoot(const struct Files *f, FILE *fp);
int WidgetTitle(const struct Files *f, FILE *fp);
