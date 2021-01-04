#ifndef _MMXF_H_
#define _MMXF_H_

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <AS_02.h>
#include <AS_02_ACES.h>

extern "C" int fill_writer_info(ASDCP::WriterInfo &info);
extern "C" int write_mxf(char *in_path, char *out_file);


#endif //_MMXF_H_
