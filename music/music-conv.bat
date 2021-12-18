@echo off
echo ---------------------------------------------------------------------
echo Beginning xm2esf conversions
echo ---------------------------------------------------------------------
cd ..
cd util
cd xm2esf

echo from Z:%1 to Z:%2

xm2esf-turbo %1 %2

REM clean up the working dir (oh god)
cd ..
