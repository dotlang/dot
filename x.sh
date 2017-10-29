#!/bin/bash

if [ -z "$0" ]; then
    echo "Usage: x.sh [name args]"
    echo "    name will run that specific test"
    echo "    args will be passed to the compiler"
    exit
fi

dotest() {
    test_file=$1
    rest_args=$2
    file_name=$(basename ${test_file})
    file_name="${file_name%.*}"

    echo "Running $test_file test..."
    echo "========================="
    #cat with line numbers
    cat -n $test_file
    echo "========================="
    ./build/dot $rest_args $test_file
    ./$file_name
    actual=$?
    expected=$(head -n 2 $test_file | tail -n 1 | cut -c3-)
    rm $file_name

    if [ "$actual" = "$expected" ]; then echo -e "\e[38;5;82m*SUCCESS! [$actual = $expected]"; else echo -e "\e[31m*FAIL! Got $actual but $expected was expected."; fi
    echo
}

cd "$(dirname "$0")"

#do we need to run a single test?
if [[ $# > 0 ]]; then
    test_file=$1
    shift
    rest_args=$*

    dotest $test_file $rest_args
    
    echo
    exit
fi

shopt -s globstar
for f in ./testsuite/**/*.dot ; do
  dotest $f
done
