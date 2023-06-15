#!/bin/bash

mkdir -p res/map

cd maps

for f in *.bin
do
	dd if=$f of=hdr.bin bs=1 count=32
	dd if=$f of=bulk.bin bs=1 skip=32
	../util/accurate-kosinski/kosinski-compress bulk.bin bulk.kos
	cat hdr.bin bulk.kos > "../res/map/${f%.bin}.map"
	rm hdr.bin bulk.bin bulk.kos
done
