#!/bin/bash
rm HeadCount.txt
touch HeadCount.txt

while :
do
	echo "Get image and detection"
	./faceDetection
	sleep 5
	echo "\n\n" 
done
