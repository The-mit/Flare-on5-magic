#!/bin/bash
# this script should automate the run-filter-rerun to a single file.
# this script will not clean after itself, you can uncomment the cleaning options

make

cp ./magic ./magic_original
./a.out > out.txt

cat out.txt | grep "Final" > filtered
cat filtered | sed 's/Final flag: //' > flag_list

# at this point the flags are 1-per-line in flag_list.
# just send them to the original ./magic
mv ./magic_original ./magic
./magic < flag_list

# cleaning
# rm ./filtered
# rm flag_list
# rm out.txt
