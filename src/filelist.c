#include "filelist.h"

#ifndef _WIN32 
    #ifndef _GNU_SOURCE 
	#define _GNU_SOURCE
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>
#include <dirent.h>

int is_dir(char *path) {
    struct stat st_in;

    if (stat(path, &st_in) != 0 ) {
        return 0;
    }

    if (S_ISDIR(st_in.st_mode)) {
        return 1;
    }

    return 0;
}

filelist_t *create_filelist(int n) {
    filelist_t *filelist;

    filelist = (filelist_t*)malloc(sizeof(filelist_t));

    filelist->nfiles = n;
    filelist->files  = (char**)malloc(filelist->nfiles * sizeof(char*));

    if (filelist->nfiles) {
        for (int i = 0; i < filelist->nfiles; i++) {
            filelist->files[i]  = (char*)malloc(MAX_FILENAME_LENGTH * sizeof(char));
        }
    }

    return filelist;
}


void free_filelist(filelist_t *filelist) {
    if (filelist == NULL) {
        return;
    }
    if (filelist->files) {
        for (int i = 0; i < filelist->nfiles; i++) {
            free(filelist->files[i]);
        }
        free(filelist->files);
    }
    free(filelist);
}

char *get_extension(const char *filename){
    return (strrchr((char*)filename, '.'));
}

int is_filter_ok(const char *filename, const char *filter){
    const char *ext = strrchr((char*)filename, '.');
    if (!ext) {
        return 0;
    }
    ext++;
    if (strcasecmp(ext, filter) == 0) {
        return 1;
    }
    return 0;
}

int get_id(char *filename) {
    //Find number string after last _
    int id = -1;
    char *num = strrchr((char*)filename, '_');
    if (!num) {
        return -1;
    }
    num++;
    sscanf(num, "%d", &id);
    return id;
}

//full name as <path>/<base>.<ext> is necessary, don't free it
char *split_filename(const char *filename, char **ppath, char **pbase, char **pext) {  
    char *result = strdup(filename);
    char *base = strrchr(result, '/');
    if (base) {
        *base++ = '\0';
        *ppath = result;
    }
    else {
        base = result;
        *ppath = (char*)".";
    }
    char *ext  = strrchr(base, '.');
    if (ext) {
        *ext++ = '\0';
    }
    *pbase = base;
    *pext = ext;
    return result; 
}

char *generate_filename(const char *path, const char *base, const char *ext) {
    char *filename = NULL;
    asprintf(&filename, "%s/%s.%s", path, base, ext);
    return filename;
}

struct tmp_file{
    char *filename;
    int id;
};

int files_cmp(const void *a, const void *b) {
    struct tmp_file *x =(struct tmp_file *)a;
    struct tmp_file *y =(struct tmp_file *)b;
    return (x->id - y->id);
}

int sort_filelist(filelist_t *list) {
    struct tmp_file *tmp = (struct tmp_file *)malloc(list->nfiles*sizeof(struct tmp_file));
    int i;
    for (i=0; i<list->nfiles; i++){
        tmp[i].filename = list->files[i];
        tmp[i].id = get_id(tmp[i].filename);
        //printf("%s --> %d\n", tmp[i].filename, tmp[i].id);
    }
    fprintf(stdout, "Sorting filelist... ");
    qsort(tmp, list->nfiles, sizeof(struct tmp_file), files_cmp);
    fprintf(stdout, "done\n");
    for (i=0; i<list->nfiles; i++){
        list->files[i]=tmp[i].filename;
    }
    free(tmp);
    return 0;
}

//TODO: sorted list instead of array with qsort
filelist_t *get_filelist(const char *path, const char *filter) {
    DIR *dir;
    struct stat st_in;
    struct dirent *entry;
    char **filenames = 0, **tmp;

    size_t cnt = 0, ln = 0;
    filelist_t *filelist;

    if (stat(path, &st_in) != 0 ) {
        fprintf(stdout, "%s is not found\n", path);
        return NULL;
    }

    if (!S_ISDIR(st_in.st_mode)) {
        fprintf(stdout, "%s is a single file\n", path);
        filelist = create_filelist(1);
        snprintf(filelist->files[0], MAX_FILENAME_LENGTH, "%s", path);
        return filelist;
    }

    if ((dir = opendir(path)) == NULL) {
        return(NULL);
    }

    fprintf(stdout, "Reading directory %s\n", path);

    while ((entry = readdir(dir))) {
    
        if (is_filter_ok(entry->d_name, filter) == 0) {
            continue;
        }
    
        if (cnt >= ln) {
            ln = 2 * ln + 1;
            if (ln > SIZE_MAX / sizeof * filenames) {
                break;
            }
            tmp = (char**)realloc(filenames, ln * sizeof * filenames);
            if (!tmp) {
                break;
            }
            filenames = tmp;
        }

        filenames[cnt] = (char*)malloc(strlen(entry->d_name) + 1);
        if (!filenames[cnt]) {
            break;
        }

        snprintf(filenames[cnt++], strlen(entry->d_name) + 1, "%s", entry->d_name);
        fprintf(stdout, "\t%s found\n", entry->d_name);
    }

    closedir(dir);
    fprintf(stdout, "Total: %lu files\n", cnt);
    
    filelist = create_filelist(cnt);

    if (filenames) {
        while (cnt-- > 0) {
            fprintf(stdout, "\t%s added\n", filenames[cnt]);
            snprintf(filelist->files[cnt], MAX_FILENAME_LENGTH, "%s/%s", path, filenames[cnt]);
            free(filenames[cnt]);
        }

        free(filenames);
    }

    return filelist;
}




