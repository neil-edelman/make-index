struct Files;
struct Recursor;

struct Files *Files(const struct Files *parent, int (*filter)(const struct Files *, const char *));
void Files_(struct Files *files);
int FilesAdvance(struct Files *files);
int FilesIsRoot(const struct Files *f);
void FilesSetPath(struct Files *files);
char *FilesEnumPath(struct Files *files);
char *FilesName(const struct Files *files);
int FilesSize(const struct Files *files);
int FilesIsDir(const struct Files *files);
