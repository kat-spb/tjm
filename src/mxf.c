#include "mxf.h"

//using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 40 * Kumu::Megabyte;
const ASDCP::Rational default_picture_rate = ASDCP::EditRate_24;

extern "C" int fill_writer_info(ASDCP::WriterInfo& info) {
    info.CompanyName = "TJM";
    info.ProductName = "TJM converter";
    info.ProductVersion = ASDCP::Version();

    //set the label type
    //info.LabelSetType = LS_MXF_UNKNOWN;
    info.LabelSetType = ASDCP::LS_MXF_SMPTE;
    fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");

    // generate a random UUID for essence
    Kumu::GenRandomUUID(info.AssetUUID);

    return 0;
}


namespace ASDCP {
  Result_t JP2K_PDesc_to_MD(const ASDCP::JP2K::PictureDescriptor& PDesc,
			    const ASDCP::Dictionary& dict,
			    ASDCP::MXF::GenericPictureEssenceDescriptor& GenericPictureEssenceDescriptor,
			    ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);
}

const ASDCP::Dictionary *g_dict =0;

extern "C" int write_mxf(char *input_path, char *output_file) {
    ASDCP::AESEncContext*  Context = 0;
    ASDCP::HMACContext* HMAC = 0;
    ASDCP::WriterInfo info;
    ASDCP::JP2K::MXFWriter Writer;
    ASDCP::JP2K::FrameBuffer FrameBuffer(FRAME_BUFFER_SIZE);
    ASDCP::JP2K::PictureDescriptor PDesc;
    ASDCP::JP2K::SequenceParser Parser;
   
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
    ASDCP::Result_t result = Parser.OpenRead(filenames.front(), true);

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
	    PictureDescriptorDump(PDesc);
#endif
   }

        //use RGB
	    ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
	    essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	  
	    result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict,
					   *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr), 				   *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));

	    if (ASDCP_SUCCESS(result)) {
	        tmp_dscr->CodingEquations = g_dict->ul(ASDCP::MDD_CodingEquations_Rec2020);
	        tmp_dscr->TransferCharacteristic = g_dict->ul(ASDCP::MDD_TransferCharacteristic_SMPTEST2084);
	        tmp_dscr->ColorPrimaries = g_dict->ul(ASDCP::MDD_ColorPrimaries_ITU2020);
	        tmp_dscr->ScanningDirection = 0;
	        tmp_dscr->PictureEssenceCoding = g_dict->ul(ASDCP::MDD_JP2KEssenceCompression_IMFProfile_8K_Reversible_7_0);
	        tmp_dscr->ComponentMaxRef = 65535;
	        tmp_dscr->ComponentMinRef = 0;

	        tmp_dscr->MasteringDisplayMinimumLuminance = 50;
		    tmp_dscr->MasteringDisplayMaximumLuminance = 40000000;
#if 0
	        if (Options.md_primaries.HasValue()) {
		        tmp_dscr->MasteringDisplayPrimaries = Options.md_primaries;
		        tmp_dscr->MasteringDisplayWhitePointChromaticity = Options.md_white_point;
		    }
#endif
	        essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);
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

        if (result == Kumu::RESULT_ENDOFFILE) {
	        result = Kumu::RESULT_OK;

            if (ASDCP_SUCCESS(result)) {
                fill_writer_info(info);
	            result = Writer.OpenWrite(output_file, info, PDesc); 
            }

            if (ASDCP_SUCCESS(result)) {
                result = Writer.Finalize();
            }
        }

    return 0;
}

