/** See \see{Files}. */
struct Files;

/** Returns a boolean value on whether {files} should include {file}. */
typedef int (*FilesFilter)(struct Files *const files, const char *file);

struct Files *Files(struct Files *const parent, const FilesFilter filter);
void Files_(struct Files *files);
int FilesAdvance(struct Files *files);
int FilesIsRoot(const struct Files *f);
void FilesSetPath(struct Files *files);
const char *FilesEnumPath(struct Files *const files);
const char *FilesName(const struct Files *const files);
int FilesSize(const struct Files *files);
int FilesIsDir(const struct Files *files);
