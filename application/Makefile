
#program name
PROGRAM = reg_dumper

#compiler
#CC = arm-bcm2708-linux-gnueabi-gcc
#CC = arm-brcm-linux-gnueabi-gcc
CC = gcc

#compiler flag
CFLAGS = -O2 -Wall

#source code
SOURCE = reg_dumper.c 

#libraries
#LIB_PATH= /usr/lib

#remove
RM = rm 

#all:	$(PROGRAM)

all:
	$(CC) $(CFLAGS) -o $(PROGRAM) $(SOURCE) -lncurses

clean:
	$(RM) -rf $(PROGRAM) *.o *.core 

