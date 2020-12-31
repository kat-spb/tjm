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

convert_to_j2k(){
    local input="$1"
    local output="$2"
    local dst
    echo "Convert data from ${input}:"
    ls -1 "${input}" | while read src; do
	prefix=`get_prefix "${src}"`
	num=`get_number "${src}"`
	dst=`join_name "${prefix}" "${num}" "j2k"`
	echo -e "\t${dst}"
	opj_compress -i "${input}/${src}" -o "${output}/${dst}"
    done
}

input="$1"
output="${input}/../`basename "${input}"`_j2k"
mkdir -p "${output}"
convert_to_j2k "${input}" "${output}"
