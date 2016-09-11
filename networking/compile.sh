#!/bin/bash
if [ $# -lt 1 ]; then
	echo 'Usage: ./compile.sh <C file path>'
	exit 1
fi
for i in $@; do
	output=${i/src/bin}
	output=${output/.c/}
	gcc $i -Iinc -o $output
done
