#!/bin/sh -e
# print the accumulated biases
# of an input stream with three
# comma-separated fields; usage is
# "head -n30000 /dev/ttyACM0 | findbias.sh"
uniq | awk -F',' '
/^A/{ax+=substr($1,2);ay+=$2;az+=$3;na++;}
/^G/{gx+=substr($1,2);gy+=$2;gz+=$3;ng++;}
/^M/{mx+=substr($1,2);my+=$2;mz+=$3;nm++;}
END{
	print "accel: "ax/na", "ay/na", "az/na;
	print "mag  : "mx/nm", "my/nm", "mz/nm;
	print "gyro : "gx/ng", "gy/ng", "gz/ng;
}'
