#!/bin/bash

get_prefix() {
    echo "$1" | sed "s|[0-9]*[.][a-zA-Z]*$||"
}

get_ext() {
    echo "$1" | sed "s|.*[.]\([a-zA-Z]*\)$|\1|"
}

get_number(){
    local src_number=`echo "$1" | sed "s|.*[_]\([0-9]*\)[.][a-zA-Z]*$|\1|" | sed "s|^0*||"`
    if [ -z "${src_number}" ]; then
        src_number=0
    fi
    echo "${src_number}"
}

join_name(){
    local prefix="$1"
    local number="$2"
    local ext="$3"
    echo "${prefix}`printf %07d ${number}`.${ext}"
}

clone() {
    local src_template="$1"
    local output_path="$2"
    local start="$3"
    local finish="$4"

    local name=`basename "${src_template}"`
    local prefix=`get_prefix "${name}"`
    #echo "prefix: ${prefix}"
    local ext=`get_ext "${name}"`
    #echo "ext: ${ext}"
    local id dst_name
    echo "Clone:"
    for ((id=start;id<=finish;id++)); do
	dst_name=`join_name "${output_path}/${prefix}" "${id}" "${ext}"`
	echo -e "\t${src_template} -> ${dst_name}"
	cp "${src_template}" "${dst_name}"
    done
}

convert_to_j2c(){
    local prefix=`get_prefix "$1"`
    local ext=`get_ext "$1"`
    local input="$2"
    local output="$3"
    local start="$4"
    local finish="$5"
    local id src_name jp2_name j2c_name
    echo "Convert to J2C:"
    for ((id=start;id<=finish;id++)); do
	src_name=`join_name "${input}/${prefix}" "${id}" "${ext}"`
	jp2_name=`join_name "${output}/${prefix}" "${id}" "jp2"`
	j2c_name=`join_name "${output}/${prefix}" "${id}" "j2c"`
	opj_compress -i "${src_name}" -o "${jp2_name}"
	mv ${jp2_name} ${j2c_name}
    done
}

convert_to_j2k(){
    local prefix=`get_prefix "$1"`
    local ext=`get_ext "$1"`
    local input="$2"
    local output="$3"
    local start="$4"
    local finish="$5"
    local id src_name j2k_name
    echo "Prepare input_j2k:"
    for ((id=start;id<=finish;id++)); do
	src_name=`join_name "${input}/${prefix}" "${id}" "${ext}"`
	j2k_name=`join_name "${output}/${prefix}" "${id}" "j2k"`
	opj_compress -i "${src_name}" -o "${j2k_name}"
    done
}

convert_to_mxf(){
    local input="$1"
    local output="$2"
    asdcp-wrap "${input}" -b 419430400 "${output}"
}

prepare_process(){
    src_template="$1"
    start="$2"
    finish="$3"

    template_path=`dirname "${src_template}"`
    template_name=`basename "${src_template}"`

    mkdir -p "${template_path}/input"
    clone "${src_template}" "${template_path}/input" "${start}" "${finish}"

    mkdir -p "${template_path}/output_j2c"
    convert_to_j2c "${template_name}" "${template_path}/input" "${template_path}/output_j2c" "${start}" "${finish}"

    mkdir -p "${template_path}/output_j2k"
    convert_to_j2k "${template_name}" "${template_path}/input" "${template_path}/output_j2k" "${start}" "${finish}"
}

read output_path
mkdir -p "${output_path}/input_j2k" "${output_path}/output_j2k" 
while read src_template start_number finish_number; do
    if [ -z "${src_template}" ]; then
	break
    fi
    prepare_process "${src_template}" "${start_number}" "${finish_number}"
    #Warning: variable ${template_xxx} updated by prepare_process
    mv "${template_path}/output_j2k/"* "${output_path}/input_j2k"
done

convert_to_mxf "${output_path}/input_j2k/" "${output_path}/output_j2k/output.mxf"