#!/bin/sh

VAR="abcdef"
echo "\$VAR = $VAR"
echo "\$0 = $0"

printf "\$0=%s\n" $0

echo $0 $1 $2 $3 {$10}

A=5
B=6
num_A=`expr $A + $B`
echo $num_A
