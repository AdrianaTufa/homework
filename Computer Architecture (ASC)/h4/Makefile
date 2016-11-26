CC=g++
CFLAGS=-g -Wall

all: accpop_gpu

accpop_gpu: host.cpp helper.cpp
	$(CC) $(CFLAGS) host.cpp helper.cpp -lOpenCL -o accpop_gpu

clean:
	rm -f opencl-bin
