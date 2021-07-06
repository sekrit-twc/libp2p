#!/bin/sh

set -e
set -x

rm -f *.o *.a *.so a.out

if [ "x$1" = "xclean" ]; then exit 0; fi

CXXFLAGS="-std=c++11 -Wall -Wextra -Wno-missing-field-initializers -O2 -fPIC"
CFLAGS="-std=c89 -Wall -Wextra -Wno-missing-field-initializers -O2"

c++ ${CXXFLAGS} -c -o p2p_api.o p2p_api.cpp
c++ ${CXXFLAGS} -c -o v210.o v210.cpp
ar rc libp2p.a p2p_api.o v210.o

cc ${CFLAGS} -L. main.c -lp2p
