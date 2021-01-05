#include <KM_fileio.h>
#include <AS_02.h>
#include <PCMParserList.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 60 * Kumu::Megabyte;
const ASDCP::Dictionary *g_dict = 0;
 
const char*
RationalToString(const ASDCP::Rational& r, char* buf, const ui32_t& len)
{
  snprintf(buf, len, "%d/%d", r.Numerator, r.Denominator);
  return buf;
}

static const char* PROGRAM_NAME = "mxf";  // program name for messages

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
} c_TJMInfo;

static void create_random_uuid(byte_t* uuidbuf)
{
  Kumu::UUID tmp_id;
  GenRandomValue(tmp_id);
  memcpy(uuidbuf, tmp_id.Value(), tmp_id.Size());
}

void usage() {
  fprintf(stdout, "USAGE: %s <inputdir> <output-file>\n\n", PROGRAM_NAME);
}

const float chromaticity_scale = 50000.0;

ui32_t set_primary_from_token(const std::string& token, ui16_t& primary){
  float raw_value = strtod(token.c_str(),0);
  if ( raw_value == 0.0 || raw_value > 1.0 ){
      fprintf(stderr, "Invalid coordinate value \"%s\".\n", token.c_str());
      return false;
    }
  primary = floor(0.5 + ( raw_value * chromaticity_scale ));
  return true;
}

const float luminance_scale = 10000.0;
//
ui32_t set_luminance_from_token(const std::string& token, ui32_t& luminance){
  float raw_value = strtod(token.c_str(),0);
    if (raw_value == 0.0 || raw_value > 400000.0 ) {
      fprintf(stderr, "Invalid luminance value \"%s\".\n", token.c_str());
      return false;
    }
  luminance = floor(0.5 + ( raw_value * luminance_scale ));
  return true;
}

#define SET_LUMINANCE(p,t)			\
  if ( ! set_luminance_from_token(t, p) ) {	\
    return false;				\
  }

//
class CommandOptions
{
  CommandOptions();

public:
  ui32_t in_duration;       // number of frames to be processed
  bool use_cdci_descriptor; // 
  Rational edit_rate;    // edit rate of JP2K sequence
  ui32_t fb_size;        // size of picture frame buffer
  bool show_ul_values_flag;    // if true, dump the UL table before going tp work.
  Kumu::PathList_t filenames;  // list of filenames to be processed

  UL channel_assignment, picture_coding, transfer_characteristic, color_primaries, coding_equations;
  ASDCP::MXF::AS02_MCAConfigParser mca_config;
  std::string language;

  ui32_t rgba_MaxRef;
  ui32_t rgba_MinRef;

  ui32_t horizontal_subsampling;
  ui32_t vertical_subsampling;
  ui32_t component_depth;
  ui8_t frame_layout;
  ASDCP::Rational aspect_ratio;
  bool aspect_ratio_flag;
  ui8_t field_dominance;
  ui32_t mxf_header_size;
  ui32_t cdci_BlackRefLevel; 
  ui32_t cdci_WhiteRefLevel;
  ui32_t cdci_ColorRange;

  ui32_t md_min_luminance, md_max_luminance;
  ASDCP::MXF::ThreeColorPrimaries md_primaries;
  ASDCP::MXF::ColorPrimary md_white_point;

  //new attributes for AS-02 support 
  AS_02::IndexStrategy_t index_strategy; //Shim parameter index_strategy_frame/clip
  ui32_t partition_space; //Shim parameter partition_spacing

    std::string out_file, profile_name; //
    
      //ST 2067-50 options
  bool aces_authoring_information_flag, aces_picture_subdescriptor_flag, target_frame_subdescriptor_flag, target_frame_index_flag;
  bool target_frame_transfer_characteristics_flag, target_frame_color_primaries_flag, target_frame_min_max_ref_flag;
  bool target_frame_viewing_environment_flag;
  std::string aces_authoring_information;
  std::string target_frame_directory;
  std::list <ui64_t> target_frame_index_list;
  UL target_frame_transfer_characteristics, target_frame_color_primaries, target_frame_viewing_environment;
  ui32_t target_frame_min_ref, target_frame_max_ref;  

  bool set_video_ref(const std::string& arg) {
    std::list<std::string> ref_tokens = Kumu::km_token_split(arg, ",");
    switch (ref_tokens.size()) {
        case 3:
        	cdci_ColorRange = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
        	ref_tokens.pop_back();
        case 2:
        	cdci_BlackRefLevel = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
        	ref_tokens.pop_back();
        case 1:
	        cdci_WhiteRefLevel = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
	        break;
        default:
	        fprintf(stderr, "Expecting <white-ref>[,<black-ref>[,<color-range>]]\n");
	        return false;
    }
    if (cdci_WhiteRefLevel > 65535 || cdci_BlackRefLevel > 65535 || cdci_ColorRange > 65535) {
        fprintf(stderr, "Unexpected CDCI video referece levels.\n");
        return false;
    }
    return true;
  }

  bool set_display_primaries(const std::string& arg)
  {
    std::list<std::string> coordinate_tokens = Kumu::km_token_split(arg, ",");
    if ( coordinate_tokens.size() != 8 )
      {
	fprintf(stderr, "Expecting four coordinate pairs.\n");
	return false;
      }

    std::list<std::string>::const_iterator i = coordinate_tokens.begin();
    if ( ! set_primary_from_token(*(i++), md_primaries.First.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.First.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Second.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Second.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Third.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Third.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_white_point.X) ) return false;
    if ( ! set_primary_from_token(*i, md_white_point.Y) ) return false;

    return true;
  }

  //
  bool set_display_luminance(const std::string& arg)
  {
    std::list<std::string> luminance_tokens = Kumu::km_token_split(arg, ",");
    if ( luminance_tokens.size() != 2 )
      {
	fprintf(stderr, "Expecting a luminance pair.\n");
	return false;
      }

    if ( ! set_luminance_from_token(luminance_tokens.front(), md_min_luminance) ) return false;
    if ( ! set_luminance_from_token(luminance_tokens.back(), md_max_luminance) ) return false;

    return true;
  }

  bool set_target_frame_min_max_code_value(const std::string& arg)
  {
    std::list<std::string> range_tokens = Kumu::km_token_split(arg, ",");
    if ( range_tokens.size() != 2 )
      {
	fprintf(stderr, "Expecting a code value pair.\n");
	return false;
      }

    target_frame_min_ref = strtol(range_tokens.front().c_str(), 0 , 10);
    target_frame_max_ref = strtol(range_tokens.back().c_str(), 0 , 10);

    return true;
  }


  bool set_target_frame_index_list(const std::string& arg, std::list<ui64_t>& r_target_frame_index_list)
  {
    std::list<std::string> index_tokens = Kumu::km_token_split(arg, ",");
    if ( index_tokens.size() == 0 )
      {
	fprintf(stderr, "Expecting at least one Target Frame Index.\n");
	return false;
      }


    std::list<std::string>::const_iterator i;
    for (i = index_tokens.begin(); i != index_tokens.end(); i++) {
    	r_target_frame_index_list.push_back(strtoll(i->c_str(), 0, 10));
    }

    return true;
  }

  CommandOptions(int argc, const char** argv) :
    use_cdci_descriptor(true),
    edit_rate(24,1), fb_size(FRAME_BUFFER_SIZE),
    show_ul_values_flag(false), partition_space(60),
    mca_config(g_dict), rgba_MaxRef(1023), rgba_MinRef(0),
    horizontal_subsampling(2), vertical_subsampling(2), component_depth(10),
    frame_layout(0), aspect_ratio(ASDCP::Rational(4,3)), aspect_ratio_flag(false), field_dominance(0),
    mxf_header_size(16384), cdci_WhiteRefLevel(940), cdci_BlackRefLevel(64), cdci_ColorRange(65536),
    md_min_luminance(0), md_max_luminance(0), target_frame_subdescriptor_flag(false),
	target_frame_index_flag(false), target_frame_transfer_characteristics_flag(false), target_frame_color_primaries_flag(false),
	target_frame_min_max_ref_flag(false), target_frame_viewing_environment_flag(false)
  {

    picture_coding = UL(g_dict->ul(MDD_JP2KEssenceCompression_IMFProfile_8K_Reversible_7_0)); 

    //set color system
    coding_equations = g_dict->ul(MDD_CodingEquations_Rec2020);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_SMPTEST2084);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU2020);
	use_cdci_descriptor = true;
    if (!set_video_ref("65535,65535,65535")) {
        fprintf(stderr, "set_video_ref() error\n");
		return;
    }
    
    if (!set_target_frame_min_max_code_value("0,65535")) {
        fprintf(stderr, "set_target_frame_min_max_code_value() error\n");
        return;
    }
    target_frame_min_max_ref_flag = true;
    rgba_MinRef = 0;  rgba_MaxRef = 65535;
    md_min_luminance = 50; md_max_luminance = 40000000;

#if 0
    if (!set_display_primaries("35400,14600,8500,39850,6550,2300,15635,16450")) {
        fprintf(stderr, "set_display_primaries() error\n");
        return;
    }
#endif

    aspect_ratio_flag = true;
	
    for (int i = 1; i < argc; i++){
	    if (argv[i][0] != '-') {
		    filenames.push_back(argv[i]);
	    }
	    else {
		    fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
		    usage();
		    return;
	    }
    }

    if (filenames.size() < 2){
	fprintf(stderr, "Option requires at least two filename arguments: <input-file> <output-file>\n");
	return;
    }

    out_file = filenames.back();
    filenames.pop_back();

  }
};

namespace ASDCP {
  Result_t JP2K_PDesc_to_MD(const ASDCP::JP2K::PictureDescriptor& PDesc,
			    const ASDCP::Dictionary& dict,
			    ASDCP::MXF::GenericPictureEssenceDescriptor& GenericPictureEssenceDescriptor,
			    ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);

  Result_t PCM_ADesc_to_MD(ASDCP::PCM::AudioDescriptor& ADesc, ASDCP::MXF::WaveAudioDescriptor* ADescObj);
}

Result_t write_file(CommandOptions& Options) {
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::JP2K::MXFWriter  Writer;
  JP2K::FrameBuffer       FrameBuffer(Options.fb_size);
  JP2K::SequenceParser    Parser;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
  ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;

    // set up essence parser
    Result_t result = Parser.OpenRead(Options.filenames.front().c_str(), true);

    // set up MXF writer
    if (ASDCP_SUCCESS(result)) {
        ASDCP::JP2K::PictureDescriptor PDesc;
        Parser.FillPictureDescriptor(PDesc);
        PDesc.EditRate = Options.edit_rate;

#if 1
	    fprintf(stderr, "JPEG 2000 pictures\n");
	    fputs("PictureDescriptor:\n", stderr);
        fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	    JP2K::PictureDescriptorDump(PDesc);
#endif

        //use RGB
	    ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
	    essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	  
	    result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict,
					   *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr),
					   *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));

	    if (ASDCP_SUCCESS(result)) {
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
        }
    }


    if (ASDCP_SUCCESS(result)){
        WriterInfo Info = c_TJMInfo;  // fill in your favorite identifiers here
        Info.LabelSetType = LS_MXF_SMPTE;

    	Kumu::GenRandomUUID(Info.AssetUUID);

        if (ASDCP_SUCCESS(result)) {
	        result = Writer.OpenWrite(Options.out_file, Info, essence_descriptor, essence_sub_descriptors, Options.edit_rate, Options.mxf_header_size, Options.index_strategy, Options.partition_space);
	    }
    }

    if (ASDCP_SUCCESS(result)) {
        ui32_t duration = 0;
        result = Parser.Reset();

        while (ASDCP_SUCCESS(result) && duration++ < Options.in_duration){
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

  return result;
}

int main(int argc, const char** argv) {
    Result_t result = RESULT_OK;

  g_dict = &ASDCP::DefaultSMPTEDict();
  assert(g_dict);
#if 0
  g_dict->Dump(stdout);
#endif


    CommandOptions Options(argc, argv);

    EssenceType_t EssenceType;
    result = ASDCP::RawEssenceType(Options.filenames.front().c_str(), EssenceType);

    if (ASDCP_SUCCESS(result)){
       result = write_file(Options);
    }

    if (ASDCP_FAILURE(result)){
      fprintf(stderr, "Error in MXF writer\n");
      return -1;
    }

    return 0;
}

