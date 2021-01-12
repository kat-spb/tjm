#ifndef _MMXF_H_
#define _MMXF_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "filelist.h"

//extern "C" int write_mxf(filelist_t *filelist, char *output_file);
int write_mxf(char *in_path, char *out_file);

#ifdef __cplusplus
}
#endif

#endif //_MMXF_H_
