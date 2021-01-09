#!/bin/bash

if [ $# -ne 4 ]; then
    echo -e "Clone @source to @output directory with the prefix and extension as source has and with the postfix number from @start to @finish.\n"
    echo -e "\tusage: $0 source output start finish\n"
    exit 0
fi

src_template="$1"
dst_dir="$2"
start="$3"
finish="$4"

mkdir -p "${dst_dir}"
echo | awk \
	-v in_file="${src_template}"    \
	-v dst_dir="${dst_dir}"         \
	-v start="${start}" \
	-v finish="${finish}"   \
'END{
    gsub(".*/", "", in_file);
    prefix = ext = in_file;
    gsub(".*[.]", "", ext);
    gsub("_[0-9]*[.][a-zA-Z0-9]*$", "", prefix);
    for(i = start; i <= finish; i++){
        printf("%s/%s_%07d.%s\n", dst_dir, prefix, i, ext);
    }
}' | while read out_file; do
    cp "${src_template}" "${out_file}"
done
