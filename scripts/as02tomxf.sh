#!/bin/bash

if [ $# -ne 2 ]; then
    echo -e "Make MXF stream from all J2K-files in  @source_dir and put result to  @output"
    echo -e "\tusage: $0 source_dir output\n"
    exit 0
fi

input="$1"
output="$2"
mkdir -p `dirname "${output}"`
as-02-wrap "${input}" -b 419430400 "${output}"