#!/bin/bash

prepare_process(){
    src_template="$1"
    start="$2"
    finish="$3"

    template_path=`dirname "${src_template}"`
    template_name=`basename "${src_template}"`

    mkdir -p "${template_path}/input"
    `dirname "$0"`/clone.sh "${src_template}" "${template_path}/input" "${start}" "${finish}"

    mkdir -p "${template_path}/input_j2k"
    /usr/local/bin/j2c "${template_name}" "${template_path}/input" "${template_path}/input_j2k" "${start}" "${finish}"
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
mkdir -p "${output_path}/input_j2k" "${output_path}/output_j2k" 
while read src_template start_number finish_number; do
    if [ -z "${src_template}" ]; then
	break
    fi
    prepare_process "${src_template}" "${start_number}" "${finish_number}"
    #Warning: variable ${template_xxx} updated by prepare_process
    #mv "${template_path}/input_j2k/"* "${output_path}/input_j2k"
done

/usr/local/bin/mxf "${output_path}/input_j2k/" "${output_path}/output_j2k/output.mxf"