#include "mxf.h"

#include <KM_fileio.h>
//#include <AS_DCP.h>
#include <AS_02.h>
#include <PCMParserList.h>
#include <Metadata.h>

using namespace ASDCP;

namespace ASDCP{
    Result_t JP2K_PDesc_to_MD(const JP2K::PictureDescriptor& PDesc, const ASDCP::Dictionary& dict,
                            ASDCP::MXF::GenericPictureEssenceDescriptor& EssenceDescriptor,
                            ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);
}

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

void set_md_primaries_P3D65(ASDCP::MXF::ThreeColorPrimaries &md_primaries,  ASDCP::MXF::ColorPrimary &md_white_point){
    //P3D65 
    //R:x=0.680y=0.320 G:x=0.265y=0.690 B:x=0.150y=0.060 
    //W:x=0.3127y=0.3290
    //devide float value to 0.00002
    md_primaries.First.X = 34000;
    md_primaries.First.Y =  16000; 
    md_primaries.Second.X = 13250;
    md_primaries.Second.Y = 34500; 
    md_primaries.Third.X = 7500; 
    md_primaries.Third.Y = 3000;
    md_white_point.X = 15635;
    md_white_point.Y = 16450;    
}


void set_md_primaries_BT2020(ASDCP::MXF::ThreeColorPrimaries &md_primaries,  ASDCP::MXF::ColorPrimary &md_white_point){
    //BT2020 
    //R:x=0.708y=0.292 G:x=0.170y=0.797 B:x=0.131y=0.046 
    //W:x=0.3127y=0.3290
    //devide float value to 0.00002
    md_primaries.First.X = 35400;
    md_primaries.First.Y =  14600;
    md_primaries.Second.X = 8500;
    md_primaries.Second.Y = 39850;
    md_primaries.Third.X = 6550;
    md_primaries.Third.Y = 2300; 
    md_white_point.X = 15635;
    md_white_point.Y = 16450;  
}

int write_mxf(char *input_path, char *output_file) {

    const ui32_t FRAME_BUFFER_SIZE = 40 * Kumu::Megabyte;
    const ASDCP::Rational default_picture_rate = Rational(24000,1001);
    const ASDCP::Dictionary *g_dict = 0;

    AESEncContext*  Context = 0;
    HMACContext* HMAC = 0;
    WriterInfo info;
    AS_02::JP2K::MXFWriter  Writer;
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

#if 0   //picture description info
	    fprintf(stdout, "JPEG 2000 pictures\n");
	    fprintf(stdout, "PictureDescriptor:\n");
	    fprintf(stdout, "Frame Buffer size: %u\n", FRAME_BUFFER_SIZE);
	    JP2K::PictureDescriptorDump(PDesc);
#endif
    }

    g_dict = &ASDCP::DefaultSMPTEDict();
    assert(g_dict);
#if 0
    g_dict->Dump(stdout);
#endif

    ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
    ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;

    MXF::LineMapPair line_map;
    line_map.First = 1;
    line_map.Second = 0;
    
    ASDCP::MXF::ThreeColorPrimaries md_primaries;
    ASDCP::MXF::ColorPrimary md_white_point;
    set_md_primaries_BT2020(md_primaries, md_white_point);
    
    ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
	essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict, 					      *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr),  					           *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));					   
    if (ASDCP_SUCCESS(result)) {
        tmp_dscr->ComponentMaxRef = 65535;
        tmp_dscr->ComponentMinRef = 0;
        tmp_dscr->FrameLayout = 0;
    	tmp_dscr->AspectRatio = ASDCP::Rational(16,9);
    	tmp_dscr->FieldDominance = 0; 
        //MasteringDisplayLuminance
        tmp_dscr->MasteringDisplayMinimumLuminance = 50;
        tmp_dscr->MasteringDisplayMaximumLuminance = 40000000;
        //MasterDisplayPrimaries
        tmp_dscr->VideoLineMap=line_map;
        tmp_dscr->MasteringDisplayPrimaries = md_primaries;
        tmp_dscr->MasteringDisplayWhitePointChromaticity = md_white_point;
    }

    //common parameters
    tmp_dscr->CodingEquations = g_dict->ul(MDD_CodingEquations_Rec2020);
    tmp_dscr->TransferCharacteristic = g_dict->ul(MDD_TransferCharacteristic_SMPTEST2084);
    tmp_dscr->ColorPrimaries = g_dict->ul(MDD_ColorPrimaries_ITU2020);
    tmp_dscr->PictureEssenceCoding = UL(g_dict->ul(MDD_JP2KEssenceCompression_IMFProfile_4K_Reversible_7_0));

    essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);

    if (ASDCP_SUCCESS(result)) {
        fill_writer_info(&info);
	    result = Writer.OpenWrite(output_file, info, essence_descriptor, essence_sub_descriptors, default_picture_rate, 16384, AS_02::IS_FOLLOW, 10);
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
