#ifndef _FILELIST_H_
#define _FILELIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILENAME_LENGTH 254

typedef struct {
    char **files;
    int  nfiles;
} filelist_t;

int is_dir(char *path);

filelist_t *create_filelist(int n);
void free_filelist(filelist_t *filelist);
filelist_t *get_filelist(const char *path, const char *filter);
int sort_filelist(filelist_t *list);

// don't free parts and free result
char *split_filename(const char *filename, char **ppath, char **pbase, char **pext);
char *generate_filename(const char *path, const char *base, const char *ext);
//split filename as <dir>/<name.ext>, don't free parts and free result
char *get_dirname(const char *filename, char **pdir);

#ifdef __cplusplus
}
#endif

#endif
