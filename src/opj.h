#ifndef _OPJ_H_
#define _OPJ_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openjpeg-2.3/openjpeg.h>

int read_image(opj_image_t **p_image, char *src);
int write_image(opj_image_t *image, const char *dst);

#ifdef __cplusplus
}
#endif

#endif 
