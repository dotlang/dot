#!/bin/bash

if [ -z "$0" ]; then
    echo "Usage: x.sh [name args]"
    echo "    name will run that specific test"
    echo "    args will be passed to the compiler"
    exit
fi

rm -rf build
mkdir build
clang++ -std=c11  `llvm-config --cflags` -x c src/dot.c `llvm-config --ldflags --libs core analysis native bitwriter --system-libs` -lm -o ./build/dot

millis(){  python -c "import time; print(int(time.time()*1000))"; }

dotest() {
    test_file=$1
    rest_args=$2
    file_name=$(basename ${test_file})
    file_name="${file_name%.*}"

    echo "$test_file..."
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


start=$(millis)

cd "$(dirname "$0")"

#do we need to run a single test?
if [[ $# > 0 ]]; then
    test_file=$1
    shift
    rest_args=$*

    dotest $test_file $rest_args
    echo -n -e '\e[?0c'
else
    shopt -s globstar
    for f in ./testsuite/**/*.dot ; do
      dotest $f
    done
fi

end=$(millis)
runtime=$(((end-start)))
tput init
echo "Total time: ${runtime}ms"
