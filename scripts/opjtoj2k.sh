#!/bin/bash

convert_one_by_one() {
    ls -1 "$1" | while read in_name; do
	out_name="${in_name/%.*/}.j2k"
	opj_compress -i "$1/${in_name}" -o "$2/${out_name}"
    done
}

convert_dir() {
    opj_compress -ImgDir "$1" \
		-IMF 4K_R,mainlevel=7,sublevel=0 \
		-threads ALL_CPUS -p CPRL -OutFor J2K
    mv "$1/*.J2K" "$2"
}

if [ $# -ne 1 ]; then
    echo -e "Convert all files from @source_dir to @output_dir"
    echo -e "\tJPEG2000 parameters: profile IMF 4k_R mainlevel 7, sublevel 0"
#    echo -e "\t                     frame_rate 24000/1001 max bitrate 4285"
    echo -e "\tusage: $0 source_dir\n"
    exit 0
fi

src_dir="$1"
dst_dir="${src_dir}_J2K"
mkdir -p "${dst_dir}"
#convert_one_by_one "${src_dir}" "${dst_dir}"
convert_dir "${src_dir}" "${dst_dir}"

