#!/bin/sh -e
#
# genUniq.sh -- Generate unique positions after <maxPly> ply
#
# Usage: genUniq.sh <maxPly>
#

if [ $# -ne 1 ]
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

tempZip=lbzip2 # Can use all cores both on OSX and Ubuntu

N=0
echo 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -,1' |
gzip -c -9 > ply.$N.csv.gz

while [ $N -le $maxPly ]
do
        if [ ! -s ply.$N.csv.gz ]
        then
                mkdir -p Temp
                echo "Expanding $M --> $N"
                gzip -c -d ply.$M.csv.gz |
                python Tools/expand.py |
                LC_ALL=C $sort -TTemp --compress-program=$tempZip |
                python Tools/combine.py |
                gzip -9 -c > ply.$N.csv.tmp
                mv ply.$N.csv.tmp ply.$N.csv.gz
        fi
        M=$N
        N=`expr $N + 1`
done
