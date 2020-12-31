#!/bin/bash

get_prefix() {
    echo "$1" | sed "s|[0-9]*[.][a-zA-Z]*$||"
}

get_ext() {
    echo "$1" | sed "s|.*[.]\([a-zA-Z]*\)$|\1|"
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
    echo "Clone ${src_template} to ${output_path}:"
    for ((id=start;id<=finish;id++)); do
	dst_name=`join_name "${output_path}/${prefix}" "${id}" "${ext}"`
	echo -e "\t"`basename "${dst_name}"`
	cp "${src_template}" "${dst_name}"
    done
}

usage() {
    echo -e "Clone @source to @output directory with the prefix and extension as source has and with the postfix number from @start to @finish.\n"
    echo -e "\tusage: $0 source output start finish\n"
}

if [ $# -ne 4 ]; then
    usage
    exit 0
fi

mkdir -p "$2"
clone "$1" "$2" "$3" "$4"