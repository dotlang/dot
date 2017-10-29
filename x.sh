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

success_count=0
fail_count=0

millis(){  python -c "import time; print(int(time.time()*1000))"; }

dotest() {
    test_file=$1
    rest_args=$2
    file_name=$(basename ${test_file})
    file_name="${file_name%.*}"

    echo -n "$test_file ..."
    ./build/dot $rest_args $test_file
    ./$file_name
    actual=$?

    before_underscore=${file_name%_}
    expected=${before_underscore#*_}
    re='^[0-9]+$'
    if ! [[ $expected =~ $re ]] ; then
        expected="0"
    fi

    rm $file_name

    if [ "$actual" = "$expected" ]; then 
        echo -e "\e[0;32m SUCCESS! [$actual = $expected]"; 
        let "success_count++"
    else 
        echo -e "\e[31m FAIL! Got $actual but $expected was expected."; 
        let "fail_count++"
    fi

    tput init
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
echo
echo "Total time: ${runtime}ms."
echo "$success_count test(s) passed, $fail_count test(s) failed."
