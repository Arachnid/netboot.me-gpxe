#!/bin/bash

function test_file {
	GSCRIPT=$1
	GSCRIPT=`echo $1 | sed "s/\//\/\//g" `
	echo -n $GSCRIPT...
	make > /dev/null || exit 1
	qemu -tftp . -cdrom bin/gpxe.iso -bootp tftp://10.0.0.2//$GSCRIPT -serial stdio > test_out 2>/dev/null &
	sleep 15
	kill $!

	#Strip out the unnecessary first lines
	awk 'BEGIN { s = 0 } /Booting from filename/ { s = 1 } { if (s == 1) print }' test_out | tail -n +3 | diff "${GSCRIPT%.gpxe}.out" - > /dev/null
	rc=$?
	if [ $rc != 0 ]
	then
		echo "Fail"
	else
		echo "Pass"
	fi
	rm test_out
	return $rc
}

FLAG=0
for FILE in tests/*.gpxe
do
	test_file $FILE
	if [ $? != 0 ]
	then
		FLAG=1
	fi
done

if [ $FLAG == 0 ]
then
	echo All tests passed!
fi
exit $FLAG
