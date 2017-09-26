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

if [[ ${1:0:1} = c* ]]; then
    target=${1:1}
    cat tests/ex$target.dot
    echo
    exit
fi

echo "**************************** Building...";
echo

make clean > /dev/null
make > /dev/null

if [[ ${1:0:1} = x* ]]; then
    target=${1:1}
    for i in `seq 1 $target`;
    do
        ./build/dot ./test/ex$i.dot &> /dev/null
        ./ex$1
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

cat -n ./test/ex$1.dot

echo
echo "**************************** Output:";
echo

./build/dot ./test/ex$1.dot
./ex$1
actual=$?
expected=$(head -n 2 test/ex$1.dot | tail -n 1 | cut -c3-)

echo
echo
echo "**************************** Result:";
echo

if [ "$actual" = "$expected" ]; then echo "SUCCESS! [$actual = $expected]"; else echo "FAIL! Got $actual but $expected was expected."; fi
echo
