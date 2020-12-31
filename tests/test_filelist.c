#include <stdio.h>
#include "filelist.h"

int main(){
    char *path;
    scanf("%ms", &path);
    filelist_t *fl = get_filelist(path, "tif");
    sort_filelist(fl);
    free_filelist(fl);
    return 0;
}
