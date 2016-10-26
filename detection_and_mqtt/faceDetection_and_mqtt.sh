#!/bin/bash
rm HeadCount.txt
touch HeadCount.txt

while :
do
	echo "Get image and detection"
	./faceDetection
	sleep 10
	echo "\n\n" 
done
