
COMPILER=gcc

ifeq ($(COMPILER),gcc)
CC = taskset -c 2 gcc
#CC = gcc
CFLAGS = -Wall -O3 -march=native -mtune=native -ftree-vectorize -D__i686__
#CFLAGS = -Wall -ggdb3 
SUFFIX = .gcc
LIBM = -lm
endif

ifeq ($(COMPILER),icc)
CC = icx
CFLAGS = -Wall -O3 -fast -axCORE-AVX2 -axCORE-AVX512 -xHost -ipo
SUFFIX = .icx
LIBM =
endif

ifeq ($(COMPILER),clang)
CC = clang-19
CFLAGS = -Wall -O3 -march=native -mtune=native
SUFFIX = .clang
LIBM = -lm
endif

mountain: mountain.c ftime.c fcycles.c fcycles.h type_defs.h
	$(CC) $(CFLAGS) -o mountain$(SUFFIX) mountain.c ftime.c fcycles.c $(LIBM)

clean:
	rm -f mountain$(SUFFIX) *.o *~


