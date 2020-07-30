# ARM Cross Compiler Makefile

# The ARM cross-compiler: Using abhiTronix's gcc optimized for Raspberry Pi's
# https://github.com/abhiTronix/raspberry-pi-cross-compilers/wiki/Cross-Compiler:-Installation-Instructions
CC = arm-linux-gnueabihf-gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#  -lm   links math library
#  -pthread   links the pthreads library
CFLAGS  = -g -Wall -O3
MATH = -lm
PTHREADS = -pthread
RM = rm

# the build target executable:
TARGET = EmbSys-Ex2

all:	$(TARGET)

$(TARGET):	main.c timer.o queue.o csv.o
	$(CC) $(CFLAGS) -o $(TARGET) $(MATH) $(PTHREADS) main.c timer.o queue.o csv.o

csv.o:  src/csv/csv.h src/csv/csv.c
	$(CC) $(CFLAGS) -c src/csv/csv.c

timer.o:    src/timer/timer.h src/timer/timer.c
	$(CC) $(CFLAGS) -c src/timer/timer.c

queue.o:    src/queue/queue.h src/queue/queue.c
	$(CC) $(CFLAGS) -c src/queue/queue.c

clean:
	$(RM) $(TARGET) queue.o timer.o csv.o
