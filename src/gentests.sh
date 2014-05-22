#!/usr/bin/env bash

set -e

declare -a test_names
declare -a files

exec 1>test-definitions.h
for d in *test*.c; do
    files[${#files[@]}]=$(basename ${d})

    for test in $(grep "int .*test.*(void *" ${d} | cut -d' ' -f2 | sed -e 's/(.*//'); do
        test_names[${#test_names[@]}]=$test
    done
done

echo "#define TESTS \\"
for (( i=0; i<${#test_names[@]}; i++ )); do
    echo "    C(${test_names[$i]}) \\"
done

echo

for (( i=0; i<${#files[@]}; i++ )); do
    echo "#include \"${files[$i]}\""
done




