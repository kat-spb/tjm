#include <AS_02.h>
#include <getopt.h>
#include "filelist.h"
#include "opj.h"
#include "mxf.h"

void usage(const char *name) {
    FILE *fd = stdout;

    fprintf(fd, "Usage:\n");
    fprintf(fd, "       %s <input_path> <output_file> \n\n", name);
    fclose(fd);
    exit(1);
}

const ASDCP::Dictionary *g_dict = NULL;

//using namespace ASDCP;

struct mxf_parameters_t{
    UL picture_coding;
};

//local program identification info written to file headers
class TJMInfo : public WriterInfo
{
public:
  TJMInfo() {
      static byte_t default_ProductUUID_Data[UUIDlen] =
      {0x7d, 0x83, 0x6e, 0x16, 0x37, 0xc7, 0x4c, 0x22,
	0xb2, 0xe0, 0x46, 0xa7, 0x17, 0xe8, 0x4f, 0x42};

      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "TJM";
      ProductName = "TJM converter";
      ProductVersion = ASDCP::Version();
  }
} cTJMInfo;

class TJMOptions {
    TJMOptions();
public:
    char *in_path;
    char *out_path;
    Kumu::PathList_t filenames;  // list of filenames to be processed
    TJMOptions(int argc, const char** argv);
    struct mxf_parameters_t params;  
};

TJMOptions::TJMOptions(int argc, const char** argv)
{
    filelist_t *filelist = NULL;
    int i, rc = -1, cnt = 0;
    char *path = NULL, *base = NULL, *ext =NULL, *src=NULL, *dst = NULL;
    opj_image_t *image = NULL;
    
    in_path = NULL;
    out_path = NULL;

    if (argc <= 1) {
        usage(argv[0]);
    }

    if (argc > 3) {
        usage(argv[0]);
    }

    in_path = (char*)argv[1];
    out_path = (char*)argv[2];
        
    //check single or dir in the path
    if (!is_dir(in_path)) {
        fprintf(stderr, "Input should be a directory but %s found\n", in_path);
        return;
    }

    char *outdir = NULL;
    char *tmp = get_dirname(out_path, &outdir);
    //check single or dir in the path
    if (!is_dir(outdir)) {
        fprintf(stderr, "Outdir should be a directory, %s found. May be the rath doesn't exist.\n", outdir);
        return;
    }

    filelist = get_filelist(in_path, "tif");
    sort_filelist(filelist);

    fprintf(stdout, "Convert files one by one to JPEG2000\n");   
    for (i = 0; i < filelist->nfiles; i++) {
        rc = 0;
        //fprintf(stderr, "Read process %s started\n", filelist->files[i]);
        rc = read_image(&image, filelist->files[i]);
        if (rc != 0) {
            fprintf(stderr, "Read %s failed\n", filelist->files[i]);
            continue;
        }
        //fprintf(stderr, "Read process %s finished\n", filelist->files[i]);
       
        src = split_filename(filelist->files[i], &path, &base, &ext);
        dst = generate_filename(outdir, base, "j2k");
        fprintf(stderr, "Write process %s started\n", dst);       
        rc = write_image(image, dst);
        //fprintf(stderr, "Write process %s finished\n", dst);
        free(dst);
        free(src);
        src = NULL;
        free(image);
        if (rc != 0) {
            fprintf(stderr, "convert failed\n");
            continue;
        }       
        fprintf(stdout, "... done\n");
        cnt++;
    }
    free(filelist);

    if (filenames.size() < 1) {
    	fprintf(stderr, "Requires at least one input file\n");
	    return;
    }
    
    //fill_UHD_HDR_parameters(&params);
    
    free(tmp);
}

int main(int argc, const char** argv) {
    //init global dictionary
    g_dict =  &ASDCP::DefaultSMPTEDict();
    assert(g_dict);
#if 0
    g_dict->Dump(stdout);
#endif

    //main process
    TJMOptions Options(argc, argv);

    //if (!write_mxf((char*)Options.in_path, (char*)Options.out_path)){
    //    fprintf(stderr, "Write MXF failed\n");
    //    return 0;
    //}

    test();
    
    return 0;
}
