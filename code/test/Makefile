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

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as

GCCDIR = ../../../gnu-decstation-ultrix/decstation-ultrix/2.95.3/

LDFLAGS = -T script -N
ASFLAGS = -mips2
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

#lab5
#INCPATH = -I../userprog 

PATH = $(GCCDIR):/lib:/usr/bin:/bin

CC = $(GCCDIR)gcc -B../../../gnu-decstation-ultrix/
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

CPP = gcc -E
INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt shell matmult sort 99table file_syscall_test user_prog_test pipe_reader pipe_wirter

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.c > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

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


# For Lab2: Test the TLB Hit Rate
99table.o: 99table.c
	$(CC) $(CFLAGS) -c 99table.c
99table: 99table.o start.o
	$(LD) $(LDFLAGS) start.o 99table.o -o 99table.coff
	../bin/coff2noff 99table.coff 99table

#lab5 syscall test
file_syscall_test.o: file_syscall_test.c
	$(CC) $(CFLAGS) -c file_syscall_test.c
file_syscall_test: file_syscall_test.o start.o
	$(LD) $(LDFLAGS) start.o file_syscall_test.o -o file_syscall_test.coff
	../bin/coff2noff file_syscall_test.coff file_syscall_test

#lab5 userprog test
user_prog_test.o: user_prog_test.c
	$(CC) $(CFLAGS) -c user_prog_test.c
user_prog_test: user_prog_test.o start.o
	$(LD) $(LDFLAGS) start.o user_prog_test.o -o user_prog_test.coff
	../bin/coff2noff user_prog_test.coff user_prog_test

#lab4 pipe
pipe_reader.o: pipe_reader.c
	$(CC) $(CFLAGS) -c pipe_reader.c
pipe_reader: pipe_reader.o start.o
	$(LD) $(LDFLAGS) start.o pipe_reader.o -o pipe_reader.coff
	../bin/coff2noff pipe_reader.coff pipe_reader

pipe_wirter.o: pipe_wirter.c
	$(CC) $(CFLAGS) -c pipe_wirter.c
pipe_wirter: pipe_wirter.o start.o
	$(LD) $(LDFLAGS) start.o pipe_wirter.o -o pipe_wirter.coff
	../bin/coff2noff pipe_wirter.coff pipe_wirter