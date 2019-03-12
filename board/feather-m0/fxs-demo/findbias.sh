#!/bin/sh -e
# print the accumulated biases
# of an input stream with three
# comma-separated fields; usage is
# "head -n30000 /dev/ttyACM0 | findbias.sh"
uniq | awk -F',' '{x+=$1;y+=$2;z+=$3;}END{print x/NR", "y/NR", "z/NR}'
