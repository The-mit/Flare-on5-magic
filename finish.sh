#!/bin/bash

make

cp ./magic ./magic_original
./a.out > out.txt

cat out.txt | grep "Final" > filtered
cat filtered | sed 's/Final flag: //' > flag_list

# at this point the flags are 1-per-line in flag_list.
# just send them to the original ./magic
mv ./magic_original ./magic
./magic < flag_list
