#include "opj.h"

#include <string.h>
#include <stdio.h>

#include <openjpeg-2.3/opj_config.h>

#include "opj/convert.h"

enum {
    FMT_TIF, //one of subborted by libtiff/openjp2
    FMT_J2K, //openjp2 JPEG2000 codestream
    FMT_J2C, //openjp2 JPEG2000 codestream (is binary equal J2K)
    FMT_JP2, //openjp2 JPEG2000 image 
    FMT_UNKNOWN  //any other
};

//callbacks
static void error_callback(const char *msg, void *client_data)
{
    (void)client_data;
    fprintf(stderr, "[ERROR] %s", msg);
}

static void warning_callback(const char *msg, void *client_data)
{
    (void)client_data;
    fprintf(stdout, "[WARNING] %s", msg);
}

static void info_callback(const char *msg, void *client_data)
{
    (void)client_data;
    fprintf(stdout, "[INFO] %s", msg);
}

int detect_format_by_extension(const char *filename) {
        static const char *extensions[] = {"tif", "j2k", "j2c", "jp2"};
        static const int format[] = {FMT_TIF, FMT_J2K, FMT_J2C, FMT_JP2};
        char *ext = (char*)strrchr(filename, '.');
        if (ext == NULL)
            return FMT_UNKNOWN;
        ext++;
        if (ext) {
            for (unsigned long int i = 0; i < sizeof(format) / sizeof(*format); i++) {
                 if(strcasecmp(ext, extensions[i]) == 0) {
                        return format[i];
                 }
            }
        } 
        return FMT_UNKNOWN;
}

int detect_format_by_magicnum(const char *filename) {
    FILE *fd = fopen(filename, "rb");
    unsigned int magic_number = 0x00000000;
    if (!fd) {
        fprintf(stderr, "Failed to open %s for reading", filename);
        return FMT_UNKNOWN;
    }
    fprintf(stdout, "Reading file header %s\n", filename);
    fread(&magic_number, sizeof(int), 1, fd);
    fclose(fd);
    fprintf(stdout, "magic_number 0x%x\n", magic_number);

    switch (magic_number) {
        //JPEG
        case 0x002A4949:
        case 0x49492A00:
            return FMT_TIF;
        case 0x000000C0:
        case 0xC0000000:
            return FMT_JP2;
        case 0xFF4FFF51:
        case 0x51FF4FFF:
            return FMT_J2K;
        default:
            return FMT_UNKNOWN;
    }
    return FMT_UNKNOWN;
}

void set_extended_encoder_parameters(int profile, opj_cparameters_t *parameters) {
    char profile_string[40] = "OPJ_PROFILE_IMF_4K";
    int mainlevel = 7;
    int sublevel = 0;
    switch (profile) {
        case OPJ_PROFILE_IMF_4K:
            strcpy(profile_string, "OPJ_PROFILE_IMF_4K");
            break;
        case OPJ_PROFILE_IMF_4K_R:
            strcpy(profile_string, "OPJ_PROFILE_IMF_4K_R");
            break;
        default:
            profile = OPJ_PROFILE_PART2;
            strcpy(profile_string, "OPJ_PROFILE_PART2");
    } 
    parameters->rsiz = (OPJ_UINT16)(profile | (sublevel << 4) | mainlevel);
    fprintf(stdout, "IMF profile %s activated\n", profile_string);
    parameters->irreversible = 0;
    //fprintf(stdout, "DWT 5-3 activeted\n");
}


int read_image(opj_image_t **p_image, char *src){
    int fmt_e = detect_format_by_extension(src);
    int fmt_h = detect_format_by_magicnum(src);
    //printf("fmt_e=%d fmt_h=%d\n", fmt_e, fmt_h);
    if ((fmt_e != fmt_h) || (fmt_e == FMT_UNKNOWN && fmt_h ==FMT_UNKNOWN)) {
        fprintf(stderr, "Strange format of source or wrong extension\n");
        return -1;
    }
    opj_cparameters_t parameters;
    switch(fmt_h) {
        case FMT_TIF:
            opj_set_default_encoder_parameters(&parameters);
            set_extended_encoder_parameters(OPJ_PROFILE_IMF_4K_R, &parameters);
            parameters.tcp_mct = (char)255;
            *p_image = tiftoimage(src, &parameters);
            return 0;
        default:
            fprintf(stderr, "Can't detect format\n");
            return -1;
    }
    return 0;  
}

static int initialize_4K_poc(opj_poc_t *POC, int numres) {
    POC[0].tile    = 1;
    POC[0].resno0  = 0;
    POC[0].compno0 = 0;
    POC[0].layno1  = 1;
    POC[0].resno1  = numres-1;
    POC[0].compno1 = 3;
    POC[0].prg1    = OPJ_CPRL;
    POC[1].tile    = 1;
    POC[1].resno0  = numres-1;
    POC[1].compno0 = 0;
    POC[1].layno1  = 1;
    POC[1].resno1  = numres;
    POC[1].compno1 = 3;
    POC[1].prg1    = OPJ_CPRL;
    return 2;
}

//TODO: check input constant cinema_profile
void set_cinema_encoder_parameters(int cinema_profile, opj_cparameters_t *parameters) {
    parameters->tile_size_on = OPJ_FALSE;
    parameters->cp_tdx=1;
    parameters->cp_tdy=1;

    /* Tile part */
    parameters->tp_flag = 'C';
    parameters->tp_on = 1;

    /* Tile and Image shall be at (0,0) */
    parameters->cp_tx0 = 0;
    parameters->cp_ty0 = 0;
    parameters->image_offset_x0 = 0;
    parameters->image_offset_y0 = 0;

    /* Codeblock size = 32*32 */
    parameters->cblockw_init = 32;
    parameters->cblockh_init = 32;
    parameters->csty |= 0x01;

    /* The progression order shall be CPRL */
    parameters->prog_order = OPJ_CPRL;

    /* No ROI */
    parameters->roi_compno = -1;

    parameters->subsampling_dx = 1;
    parameters->subsampling_dy = 1;

    /* 9-7 transform */
    parameters->irreversible = 1;

    parameters->tcp_rates[0] = 0;
    parameters->tcp_numlayers++;
    parameters->cp_disto_alloc = 1;

    parameters->rsiz = OPJ_PROFILE_CINEMA_2K;

    if (cinema_profile == OPJ_PROFILE_CINEMA_4K) {
        parameters->rsiz = OPJ_PROFILE_CINEMA_4K;
        parameters->numpocs = initialize_4K_poc(parameters->POC,parameters->numresolution);
    }
}

int write_image(opj_image_t *image, const char *dst){
    int rc = -1;
#if 0
    int max_comp_size;
    int max_cs_len;
#endif
    opj_cparameters_t parameters;
    opj_codec_t* l_codec = 0;
    opj_stream_t *l_stream = 0;
    int fmt = detect_format_by_extension(dst);

    opj_set_default_encoder_parameters(&parameters);
    set_extended_encoder_parameters(OPJ_PROFILE_IMF_4K_R, &parameters);
    parameters.tcp_mct = (image->numcomps >= 3) ? 1 : 0;

    //TODO: place to think about J2K/J2C
    //fprintf(stdout, "Creating compressor for %s\n", dst);
    switch (fmt) {
        case FMT_J2K: { //JPEG2000 codestream
            l_codec = opj_create_compress(OPJ_CODEC_J2K);
            break;
        }
        //the sample .j2c has the 85-bytes header, but bu openjpeg j2c vs j2k binary equal
        case FMT_J2C: 
        case FMT_JP2: { //JPEG2000 compressed image data
            l_codec = opj_create_compress(OPJ_CODEC_JP2);
            break;
        }
        default:
            fprintf(stderr, "Unknown parameter format, skipping file %s\n", dst);
        }

    if (!l_codec) {
        fprintf(stderr, "Problems with codec for %s\n", dst);
    }       

    //catch events using our callbacks and give a local context 
    opj_set_info_handler(l_codec, info_callback, NULL);
    opj_set_warning_handler(l_codec, warning_callback, NULL);
    opj_set_error_handler(l_codec, error_callback, NULL);
 
    //fprintf(stdout, "Setting up openjpeg encoder\n");
    image->color_space = OPJ_CLRSPC_SRGB;
    if (!opj_setup_encoder(l_codec, &parameters, image)) {
        fprintf(stderr, "Encode image failed\n");
        opj_destroy_codec(l_codec);
    }

    //fprintf(stdout, "Opening output stream\n");
    l_stream = opj_stream_create_default_file_stream(dst, OPJ_FALSE);

    if (!l_stream){
        fprintf(stderr, "Open output stream failed\n");
        opj_destroy_codec(l_codec);
        return -1;
    }

    //fprintf(stdout,"Starting compression %s\n", dst);
    rc = opj_start_compress(l_codec, image, l_stream);
    if (!rc) {
        fprintf(stdout, "Unable to start compression for %s\n", dst);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        return -1;
    }

    fprintf(stdout, "Starting encoding %s\n", dst);
    rc = opj_encode(l_codec, l_stream);
    if (!rc) {
        fprintf(stderr, "Unable to encode file %s\n", dst);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        return -1;
    }

    fprintf(stdout, "Finishing compression %s\n", dst);
    rc = opj_end_compress(l_codec, l_stream);
    if (!rc) {
        fprintf(stderr, "Unable to finish compression for %s\n", dst);
    }

    fprintf(stdout, "Encoding for %s complete\n", dst);
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    return 0;  
}


