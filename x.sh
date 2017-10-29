#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: x.sh cn | xn | n"
    echo "    Where n is a test number (corresponding to file ex{n}.dot in test dir"
    echo "    n will run that specific test"
    echo "    cn will show the test file"
    echo "    xn will run all tests up to n"
    exit
fi

cd "$(dirname "$0")"

number=$1
shift
if [[ ${number:0:1} = c* ]]; then
    target=${number:1}
    cat tests/ex$target.dot
    echo
    exit
fi

echo "**************************** Building...";
echo

rm -rf build
mkdir build
clang++ -std=c11  `llvm-config --cflags` -x c src/dot.c `llvm-config --ldflags --libs core analysis native bitwriter --system-libs` -lm -o ./build/dot

if [[ ${number:0:1} = x* ]]; then
    target=${number:1}
    for i in `seq 1 $target`;
    do
        ./build/dot ./test/ex$i.dot &> /dev/null
        ./ex$i
        actual=$?
        expected=$(head -n 2 test/ex$i.dot | tail -n 1 | cut -c3-)
        echo -n "$i >> ";
        if [ "$actual" = "$expected" ]; then echo "SUCCESS! [$actual = $expected]"; else echo "FAIL! Got $actual but $expected was expected."; fi
    done
    echo
    exit
fi

echo
echo "**************************** Input file:";
echo

cat -n ./test/ex$number.dot

echo
echo "**************************** Output:";
echo

rest_args=$*
echo "rest of args: $rest_args"
echo "./build/dot $rest_args test/ex$number.dot"
./build/dot $rest_args test/ex$number.dot
./ex$number
actual=$?
expected=$(head -n 2 test/ex$number.dot | tail -n 1 | cut -c3-)
rm ex$number

echo
echo
echo "**************************** Result:";
echo

if [ "$actual" = "$expected" ]; then echo "SUCCESS! [$actual = $expected]"; else echo "FAIL! Got $actual but $expected was expected."; fi
echo
