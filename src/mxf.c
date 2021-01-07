#include "mxf.h"

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <PCMParserList.h>
#include <Metadata.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 40 * Kumu::Megabyte;
const ASDCP::Rational default_picture_rate = EditRate_23_98; //EditRate_24;
const ASDCP::Dictionary *g_dict = 0;

Result_t fill_writer_info(WriterInfo *info) {
    Result_t  result = RESULT_OK;

    info->CompanyName = "TJM";
    info->ProductName = "TJM converter";
    info->ProductVersion = ASDCP::Version();

    //set the label type
    //info->LabelSetType = LS_MXF_UNKNOWN;
    info->LabelSetType = LS_MXF_SMPTE;
	fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");

    // generate a random UUID for essence
    Kumu::GenRandomUUID(info->AssetUUID);

    return result;
}

extern "C" int write_mxf(char *input_path, char *output_file) {
    AESEncContext*  Context = 0;
    HMACContext* HMAC = 0;
    WriterInfo info;
    JP2K::MXFWriter Writer;
    JP2K::FrameBuffer FrameBuffer(FRAME_BUFFER_SIZE);
    JP2K::PictureDescriptor PDesc;
    JP2K::SequenceParser Parser;
   
    ui32_t in_duration = 0xffffffff; 
    std::string out_file = std::string(output_file);  
    Kumu::PathList_t filenames;  // list of filenames to be processed      
    
    filenames.push_back(std::string(input_path));

    if (filenames.size() < 1) {
    	fprintf(stderr, "Requires at least one input file\n");
	    return -1;
    }

    //set up essence parser
    Result_t result = Parser.OpenRead(filenames.front(), true);

    //set up MXF writer
    if (ASDCP_SUCCESS(result)) {
        Parser.FillPictureDescriptor(PDesc);
        PDesc.EditRate = default_picture_rate;
#if 0
        if (PDesc.StoredWidth < 2048) {
            fprintf(stderr, "P-HFR files currently limited to 2K.\n");
            return RESULT_FAIL;
        }
#endif

#if 1 //picture description info
	fprintf(stdout, "JPEG 2000 pictures\n");
	fprintf(stdout, "PictureDescriptor:\n");
	fprintf(stdout, "Frame Buffer size: %u\n", FRAME_BUFFER_SIZE);
	JP2K::PictureDescriptorDump(PDesc);
    }
#endif

    g_dict = &ASDCP::DefaultSMPTEDict();

    ASDCP::MXF::RGBAEssenceDescriptor *tmp_rgba = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
    tmp_rgba->CodingEquations = g_dict->ul(MDD_CodingEquations_Rec2020);
    tmp_rgba->TransferCharacteristic = g_dict->ul(MDD_TransferCharacteristic_SMPTEST2084);
    tmp_rgba->ColorPrimaries = g_dict->ul(MDD_ColorPrimaries_ITU2020);
    tmp_rgba->PictureEssenceCoding = UL(g_dict->ul(MDD_JP2KEssenceCompression_IMFProfile_8K_Reversible_7_0));
    tmp_rgba->ComponentMaxRef = 65535;
    tmp_rgba->ComponentMinRef = 0;
    tmp_rgba->MasteringDisplayMinimumLuminance = 50;
    tmp_rgba->MasteringDisplayMaximumLuminance = 40000000;

    if (ASDCP_SUCCESS(result)) {
        fill_writer_info(&info);
	result = Writer.OpenWriteCustom(output_file, info, PDesc, 16384, tmp_rgba);
    }

    delete tmp_rgba;

    if (ASDCP_SUCCESS(result)) {
        ui32_t duration = 0;
        result = Parser.Reset();

        while (ASDCP_SUCCESS(result) && duration++ < in_duration){
	    result = Parser.ReadFrame(FrameBuffer);
	    if (ASDCP_SUCCESS(result)) {
    	        result = Writer.WriteFrame(FrameBuffer, Context, HMAC);
    	    }
        }
    }

    if (result == RESULT_ENDOFFILE) {
	result = RESULT_OK;
    }

    if (ASDCP_SUCCESS(result)) {
        result = Writer.Finalize();
    }

    return 0;
}
