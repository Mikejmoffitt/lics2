#!/bin/bash

mkdir -p res/map

cd maps

for f in *.bin
do
	../util/accurate-kosinski/kosinski-compress "$f" "../res/map/${f%.bin}.kos"
	echo $f
done
