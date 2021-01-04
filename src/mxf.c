#include "mxf.h"

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <PCMParserList.h>


using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 40 * Kumu::Megabyte;
const ASDCP::Rational default_picture_rate = EditRate_24;

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

namespace ASDCP {
  Result_t JP2K_PDesc_to_MD(const ASDCP::JP2K::PictureDescriptor& PDesc,
			    const ASDCP::Dictionary& dict,
			    ASDCP::MXF::GenericPictureEssenceDescriptor& GenericPictureEssenceDescriptor,
			    ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);
}

const ASDCP::Dictionary *g_dict =0;

int write_mxf(char *input_path, char *output_file) {
    AESEncContext*  Context = 0;
    HMACContext* HMAC = 0;
    WriterInfo info;
    JP2K::MXFWriter Writer;
    JP2K::FrameBuffer FrameBuffer(FRAME_BUFFER_SIZE);
    JP2K::PictureDescriptor PDesc;
    JP2K::SequenceParser Parser;
   
    ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
    ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;

    ui32_t in_duration = 0xffffffff; 
    std::string out_file = std::string(output_file);  
    Kumu::PathList_t filenames;  // list of filenames to be processed      
    
    filenames.push_back(std::string(input_path));

    if (filenames.size() < 1) {
    	fprintf(stderr, "Requires at least one input file\n");
	    return -1;
    }

    g_dict = &ASDCP::DefaultSMPTEDict();
    assert(g_dict);

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
        //use RGB
	    ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
	    essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	  
	    result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict,
					   *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr), 				   *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));

	    if (ASDCP_SUCCESS(result)) {
#if 0
	        tmp_dscr->CodingEquations = Options.coding_equations;
	        tmp_dscr->TransferCharacteristic = Options.transfer_characteristic;
	        tmp_dscr->ColorPrimaries = Options.color_primaries;
	        tmp_dscr->ScanningDirection = 0;
	        tmp_dscr->PictureEssenceCoding = Options.picture_coding;
	        tmp_dscr->ComponentMaxRef = Options.rgba_MaxRef;
	        tmp_dscr->ComponentMinRef = Options.rgba_MinRef;

	        if (Options.md_min_luminance || Options.md_max_luminance ) {
	            tmp_dscr->MasteringDisplayMinimumLuminance = Options.md_min_luminance;
		        tmp_dscr->MasteringDisplayMaximumLuminance = Options.md_max_luminance;
		    }

	        if (Options.md_primaries.HasValue()) {
		        tmp_dscr->MasteringDisplayPrimaries = Options.md_primaries;
		        tmp_dscr->MasteringDisplayWhitePointChromaticity = Options.md_white_point;
		    }
	        essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);
#endif
        }
    

        if (ASDCP_SUCCESS(result)) {
            fill_writer_info(&info);
	        result = Writer.OpenWrite(output_file, info, PDesc); 
        }

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

int test(){
r   eturn 0;
}

