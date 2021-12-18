#!/bin/bash
rm -rf ../res/mus/*
rm -rf ./out
mkdir -p ./out

# Convert samples
for s in `find raw_samples/* -type f -printf '%f '`
do
	out_name=$(echo "$s" | cut -f 1 -d '.')
	../util/pcm2ewf-lin64 raw_samples/$s out/$out_name.ewf &
done

# Convert VGI instruments
for i in `find instruments/*.vgi -type f -printf '%f '`
do
	out_name=$(echo "$i" | cut -f 1 -d '.')
	../util/vgi2eif-lin64 instruments/$i out/$out_name.eif &
done

# Just copy envelopes, I guess
cp psg/*.eef out/

# Convert XM files with WINE
for d in `find xm/* -type d -printf '%f '`
do
	in_raw=$(pwd)/xm/$d/$d.xif
	in_arg="${in_raw//\//\\}"
	out_raw=$(pwd)/out/$d.esf
	out_arg="${out_raw//\//\\}"
	wine cmd /C music-conv.bat $in_arg $out_arg
done

# Copy output to res
mkdir -p ../res/mus
mkdir -p ../res/mus/ewf
mkdir -p ../res/mus/esf
mkdir -p ../res/mus/eif
mkdir -p ../res/mus/eef
cp out/*.ewf ../res/mus/ewf/
cp out/*.esf ../res/mus/esf/
cp out/*.eif ../res/mus/eif/
cp out/*.eef ../res/mus/eef/
