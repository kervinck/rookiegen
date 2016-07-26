#!/bin/sh -e
#
# genUniq.sh -- Generate unique positions after <maxPly> ply
#
# Usage: genUniq.sh <maxPly>
#

if [ $# -ne 2 ]
then
        echo "$0: Argument error" 2>&1
        exit
fi
maxPly=$1

sort=sort
if [ `uname -s` = "Darwin" ]
then
        sort=gsort # For --compress-program option
fi

tempZip=lbzip2     # Can use all cores both on OSX and Ubuntu
export LBZIP2=-2   # A low level to get small chunks and therefore many cores
bzipBuffer=1000000

N=0
echo 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -,1' |
bzip2 -c -9 > ply.$N.csv.bz2

while [ $N -le $maxPly ]
do
        if [ ! -s ply.$N.csv.bz2 ]
        then
                echo "Expanding $M --> $N"
                mkdir -p Temp

                lbzip2 -c -d ply.$M.csv.bz2 |
                python Tools/expand.py |
                LC_ALL=C $sort -TTemp --compress-program=$tempZip --buffer-size=$bzipBuffer |
                python Tools/combine.py |
                lbzip2 -1 -c > ply.$N.csv.tmp

                mv ply.$N.csv.tmp ply.$N.csv.bz2
        fi
        M=$N
        N=`expr $N + 1`
done
