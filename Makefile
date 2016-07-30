
#-----------------------------------------------------------------------
#
#       Makefile for Linux and OSX (type 'make help' for an overview)
#
#-----------------------------------------------------------------------

# If the default compiler doesn't support --std=c11 yet, install gcc-4.8
# and type `make CC=gcc-4.8'

#-----------------------------------------------------------------------
#       Definitions
#-----------------------------------------------------------------------

perftSources:=generate.c layout.c attack.c move.c capture.c promote.c castle.c enpassant.c exchange.c rmoves.c cplus.c
perftSources:=$(addprefix Source/, $(perftSources))

combineSources:=combine.c cplus.c
combineSources:=$(addprefix Source/, $(combineSources))

expandSources:=generate.c layout.c attack.c move.c capture.c promote.c castle.c enpassant.c exchange.c expand.c cplus.c format.c
expandSources:=$(addprefix Source/, $(expandSources))

osType:=$(shell uname -s)

CFLAGS:=-std=c11 -Wall -Wextra -O3 -fstrict-aliasing -fomit-frame-pointer

ifeq "$(osType)" "Linux"
 LDFLAGS:=-lm -lpthread
endif

#-----------------------------------------------------------------------
#       Targets
#-----------------------------------------------------------------------

all: combine rmoves expand

data.c: makeData
	./makeData > data.c

rmoves: $(wildcard Source/*) data.c Makefile
	$(CC) $(CFLAGS) -o $@ $(perftSources) data.c $(LDFLAGS)

combine: $(wildcard Source/*) Makefile
	$(CC) $(CFLAGS) -o $@ $(combineSources) $(LDFLAGS)

expand: $(wildcard Source/*) data.c Makefile
	$(CC) $(CFLAGS) -o $@ $(expandSources) data.c $(LDFLAGS)

makeData: Source/makeData.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: rmoves
	Tools/genUniq.sh 4
	gzip -c -d ply.4.csv.gz | ./rmoves 4

clean:
	rm -f makeData data.c
	rm -f ply.*.csv.*

#-----------------------------------------------------------------------
#
#-----------------------------------------------------------------------
# vi: noexpandtab
