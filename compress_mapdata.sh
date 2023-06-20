#!/bin/bash

mkdir -p res/map
mkdir -p /tmp/mapwork

cd maps

for f in *.bin
do
(
	dd if=$f of=/tmp/mapwork/${f%.bin}.hdr bs=1 count=32
	dd if=$f of=/tmp/mapwork/${f%.bin}.dat bs=1 skip=32
	../util/accurate-kosinski/kosinski-compress /tmp/mapwork/${f%.bin}.dat /tmp/mapwork/${f%.bin}.kos
	cat /tmp/mapwork/${f%.bin}.hdr /tmp/mapwork/${f%.bin}.kos > "../res/map/${f%.bin}.map" ) &
done

wait

rm -rf /tmp/mapwork
