#!/bin/bash
base_dir=`dirname "$0"`

prepare_process(){
    src_template="$1"
    output_path="$2"
    start="$3"
    finish="$4"

    mkdir -p "${output_path}/input" "${output_path}/input_j2k"
    mkdir -p "${output_path}/tmp_clone" "${output_path}/tmp_j2k"

    "${base_dir}/clone.sh" "${src_template}" "${output_path}/tmp_clone" "${start}" "${finish}"
    j2c -i "${output_path}/tmp_clone" -o "${output_path}/tmp_j2k"

    mv "${output_path}/tmp_clone/"* "${output_path}/input/"
    mv "${output_path}/tmp_j2k/"* "${output_path}/input_j2k/"
    rm -rf "${output_path}/tmp_clone" "${output_path}/tmp_j2k"
}

if [ $# -ne 0 ]; then
    echo -e "Cloning TIF files by templates, converting from to JPEG2000 codestream format and conteinering restuls to MXF\n"
    echo -e "\tusage: $0 < input.txt\n"
    echo -e "\n Format of \'input.txt\':\n"
    echo -e "\t 1st line: <output directory full path>"
    echo -e "\t next lines: <first template with path> <result start number> <result finish number>\n"
    exit 0
fi

read output_path
while read src_template start_number finish_number; do
    if [ -z "${src_template}" ]; then
	break
    fi
    prepare_process "${src_template}" "${output_path}" "${start_number}" "${finish_number}"
done

mxf -i "${output_path}/input_j2k/" -o "${output_path}/output.mxf"

