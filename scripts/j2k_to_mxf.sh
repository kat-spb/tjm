#!/bin/bash

input="$1"
output="$2"
mkdir -p `dirname "${output}"`
asdcp-wrap "${input}" -b 419430400 "${output}"