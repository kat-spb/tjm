#include <getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OPENMP
#include <omp.h>
#endif

#include "filelist.h"
#include "opj.h"

//TODO: use glog

void usage(char *name) {
    FILE *fd = stdout;

    fprintf(fd, "Usage:\n");
    fprintf(fd, "       %s -i <file> -o <file> [options ...]\n\n", name);
    fprintf(fd, "Required:\n");
    fprintf(fd, "       -i | --input <file>            - input file or directory\n");
    fprintf(fd, "       -o | --output <file>           - output file or directory\n");
    fprintf(fd, "\n");
    fprintf(fd, "Options:\n");
    fprintf(fd, "       -s | --start                   - start frame\n");
    fprintf(fd, "       -f | --finish                  - end frame\n");
    fprintf(fd, "       -h | --help                    - show help\n");
    fprintf(fd, "\n\n");

    fclose(fd);
    exit(1);
}

void print_progress(int val, int total) {
    int step = 20;
    double c = (double)step / total * (double)val;
#ifdef OPENMP
    int nthreads = omp_get_num_threads();
#else
    int nthreads = 1;
#endif
    fprintf(stdout, "  Conversion use %d thread", nthreads);
    if (nthreads > 1) {
        printf("s");
    }
    fprintf(stdout, ") [");

    for (int i = 0; i < step; i++) {
        if (c > i) {
            fprintf(stdout, "=");
        }
        else {
            fprintf(stdout, " ");
        }
    }
    fprintf(stdout, "] 100%% [%d/%d]\r", val, total);
    fflush(stdout);
}

int main (int argc, char **argv) {
    char *in_path  = NULL;
    char *out_path = NULL;
    filelist_t *filelist;

    if (argc <= 1) {
        usage(argv[0]);
    }
    
    int start_frame = -1, end_frame = -1;

    //parse options
    while (1)
    {
        static struct option long_options[] =
        {
            {"help",           required_argument, 0, 'h'},
            {"input",          required_argument, 0, 'i'},
            {"output",         required_argument, 0, 'o'},
            {"start",          required_argument, 0, 's'},
            {"finish",         required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long (argc, argv, "i:o:s:f:h",
                         long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c)
        {
            case 0:
                if (long_options[option_index].flag != 0) {
                    break;
                }
                break;

            case 'i':
                in_path = optarg;
                break;

            case 'o':
                out_path = optarg;
                break;

            case 's':
                start_frame = strtol(optarg, NULL, 10);
                if (start_frame < 0) {
                    fprintf(stderr, "Start frame num should be more then 0\n");
                    return 0;
                }
                printf("start_frame = %d\n", start_frame);
                break;

            case 'f':
                end_frame = strtol(optarg, NULL, 10);
                if (end_frame < 0) {
                    fprintf(stderr, "Finish frame num should be more then 0\n");
                    return 0;
                }
                printf("end_frame = %d\n", end_frame);
                break;

            case 'h':
                usage(argv[0]);
                break;

        }
    }

    //check input path
    if (in_path == NULL) {
        fprintf(stderr, "Missing input file path\n");
        return 0;
    }

    if (in_path[strlen(in_path) - 1] == '/') {
        in_path[strlen(in_path) - 1] = '\0';
    }

    //check output path
    if (out_path == NULL) {
        fprintf(stderr, "Missing output file path\n");
        return 0;
    }

    if (out_path[strlen(out_path) - 1] == '/') {
        out_path[strlen(out_path) - 1] = '\0';
    }

    //possible directory or single file in the path
    if (!is_dir(in_path)) {
        //get_path(in_path);
        fprintf(stdout, "Input is not a directory, use path %s\n", in_path);
    }
    if (!is_dir(out_path)) {
        //get_path(out_path);
        fprintf(stderr, "Output is not a directory, use path %s\n", out_path);
    }

    fprintf(stdout, "Searching path %s\n", in_path);
    filelist = get_filelist(in_path, "tif");
    sort_filelist(filelist);


    if (filelist == NULL || filelist->nfiles < 1) {
        fprintf(stdout, "No input files located\n");
    }
    
    if (start_frame == -1) {
        fprintf(stdout, "Start frame is not defined, set it to 0\n");
        start_frame = 0;
    }

    if (end_frame > filelist->nfiles) {
         fprintf(stdout, "Finish frame is greater than the actual frame count, set it to last\n");
         end_frame = filelist->nfiles;
    }
    else if (end_frame == -1) {
         fprintf(stdout, "Finish frame is not defined, set it to last\n");
         end_frame = filelist->nfiles;
    }
    
    if (start_frame > end_frame) {
         fprintf(stderr, "Finish frame num should be more then start frame num\n");
         return 0;
    }
    
    fprintf(stdout, "Convert files one by one to JPEG2000\n");   
    int i, rc = -1, cnt = 0;
    char *path = NULL, *base = NULL, *ext =NULL, *src=NULL, *dst = NULL;
    opj_image_t *image = NULL;

#if 0
#ifdef OPENMP
    openmp_flag = 1;
    openmp_threads = omp_get_num_procs();
    omp_set_num_threads(openmp_threads);
    fprintf(stdout, "OpenMP Enabled\n");
#endif

    #pragma omp parallel for private(i)    
#endif
    for (i = start_frame; i < end_frame; i++) {
#if 0
        #pragma omp flush(SIGINT_received)
#endif
        rc = 0;
        //fprintf(stderr, "Read process %s started\n", filelist->files[i]);
        rc = read_image(&image, filelist->files[i]);
        if (rc != 0) {
            fprintf(stderr, "Read %s failed\n", filelist->files[i]);
            continue;
        }
        //fprintf(stderr, "Read process %s finished\n", filelist->files[i]);
        
        src = split_filename(filelist->files[i], &path, &base, &ext);   
        dst = generate_filename(out_path, base, "j2k");
        fprintf(stderr, "Write process %s started\n", dst);       
        rc = write_image(image, dst);
        //fprintf(stderr, "Write process %s finished\n", dst);
        free(dst);
#if 0
        dst = generate_filename(out_path, base, "j2c");
        rc |= write_image(image, dst);
        free(dst);

        dst = generate_filename(out_path, base, "jp2");
        rc |= write_image(image, dst);
        free(dst);
#endif
        free(src);
        src = NULL;
        free(image);
        if (rc != 0) {
            fprintf(stderr, "convert failed\n");
            continue;
        }       
        fprintf(stdout, "... done\n");
        cnt++;
        //print_progress(cnt, end_frame);
    }

    free_filelist(filelist);

    return 0;
}
