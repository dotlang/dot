#!/bin/bash

if [ -z "$0" ]; then
    echo "Usage: x.sh [name args]"
    echo "    name will run that specific test"
    echo "    args will be passed to the compiler"
    exit
fi

cd "$(dirname "$0")"
rm -rf build
mkdir build
# -g to generate debug info
clang++ -Werror -std=c11  `llvm-config --cflags` -g -x c -I src src/*.c `llvm-config --ldflags --libs core analysis native bitwriter --system-libs` -lm -o ./build/dot
compile_result=$?
if [ $compile_result -ne "0" ]; then
    exit
fi


success_count=0
fail_count=0

millis(){  python -c "import time; print(int(time.time()*1000))"; }

dotest() {
    test_file=$1
    rest_args=$2
    file_name=$(basename ${test_file})
    file_name="${file_name%.*}"

    # echo "./build/dot $rest_args $test_file"
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
        echo -e "\e[0;32m${test_file} \t PASSED!"
        let "success_count++"
    else 
        echo -e "\e[31m${test_file} \t FAILED! Got $actual but $expected was expected."
        let "fail_count++"
    fi

    tput init
}


echo
start=$(millis)

#do we need to run a single test?
if [[ $# > 0 ]]; then
    test_file=$1
    shift
    rest_args=$*

    dotest $test_file $rest_args
else
    shopt -s globstar
    for f in ./testsuite/**/*.dot ; do
      dotest $f
    done
fi

end=$(millis)
runtime=$(((end-start)))
echo
echo "Total time: ${runtime}ms"
echo -e "$success_count test(s) passed, $fail_count test(s) failed"
