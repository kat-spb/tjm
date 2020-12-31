#ifndef _FILELIST_H_
#define _FILELIST_H_

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

//full name as <path>/<base>.<ext> is necessary, don't free it
char *split_filename(const char *filename, char **ppath, char **pbase, char **pext);
char *generate_filename(const char *path, const char *base, const char *ext);

#endif
