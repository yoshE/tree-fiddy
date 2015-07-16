# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = 
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR = 
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles forktest Exec_Test exectest runsim Airport memoryTest

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles

forktest.o: forktest.c
	$(CC) $(CFLAGS) -c forktest.c
forktest: forktest.o start.o
	$(LD) $(LDFLAGS) start.o forktest.o -o forktest.coff
	../bin/coff2noff forktest.coff forktest

Exec_Test.o: Exec_Test.c
	$(CC) $(CFLAGS) -c Exec_Test.c
Exec_Test: Exec_Test.o start.o
	$(LD) $(LDFLAGS) start.o Exec_Test.o -o Exec_Test.coff
	../bin/coff2noff Exec_Test.coff Exec_Test
	
	
exectest.o: exectest.c
	$(CC) $(CFLAGS) -c exectest.c
exectest: exectest.o start.o
	$(LD) $(LDFLAGS) start.o exectest.o -o exectest.coff
	../bin/coff2noff exectest.coff exectest

runsim.o: runsim.c
	$(CC) $(CFLAGS) -c runsim.c
runsim: runsim.o start.o
	$(LD) $(LDFLAGS) start.o runsim.o -o runsim.coff
	../bin/coff2noff runsim.coff runsim
	
Airport.o: Airport.c
	$(CC) $(CFLAGS) -c Airport.c
Airport: Airport.o start.o
	$(LD) $(LDFLAGS) start.o Airport.o -o Airport.coff
	../bin/coff2noff Airport.coff Airport
	
memoryTest.o: memoryTest.c
	$(CC) $(CFLAGS) -c memoryTest.c
memoryTest: memoryTest.o start.o
	$(LD) $(LDFLAGS) start.o memoryTest.o -o memoryTest.coff
	../bin/coff2noff memoryTest.coff memoryTest
	
clean:
	rm -f *.o *.coff
	rm halt sort matmult testfiles forktest exectest runsim Airport memoryTest
	rm *~
