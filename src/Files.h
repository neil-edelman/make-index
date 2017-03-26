/** See \see{Files}. */
struct Files;

/** Returns a boolean value. */
typedef int (*FilesFilter)(const struct Files *, const char *);

struct Files *Files(const struct Files *parent, const FilesFilter filter);
void Files_(struct Files *files);
int FilesAdvance(struct Files *files);
int FilesIsRoot(const struct Files *f);
void FilesSetPath(struct Files *files);
char *FilesEnumPath(struct Files *files);
char *FilesName(const struct Files *files);
int FilesSize(const struct Files *files);
int FilesIsDir(const struct Files *files);
