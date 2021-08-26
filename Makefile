MY_CFLAGS := -O2 -fPIC $(CFLAGS)
MY_CXXFLAGS := -std=c++14 -O2 -fPIC $(CXXFLAGS)
MY_CPPFLAGS := $(CPPFLAGS)
MY_LDFLAGS := $(LDFLAGS)
MY_LIBS := $(LIBS)

libp2p_HDRS = \
	p2p.h \
	p2p_api.h \
	simd/cpuinfo_x86.h \
	simd/p2p_simd.h

libp2p_OBJS = \
	p2p_api.o \
	simd/cpuinfo_x86.o \
	simd/p2p_simd.o \
	simd/p2p_sse41.o

ifeq ($(SIMD), 1)
  simd/p2p_sse41.o: EXTRA_CXXFLAGS := -msse4.1
  MY_CPPFLAGS := -DP2P_SIMD $(MY_CPPFLAGS)
endif

all: libp2p.a

libp2p.a: $(libp2p_OBJS)
	ar rcs $@ $^

%.o: %.cpp $(znedi3_HDRS) $(testapp_HDRS) $(vsxx_HDRS)
	$(CXX) -c $(EXTRA_CXXFLAGS) $(MY_CXXFLAGS) $(MY_CPPFLAGS) $< -o $@

clean:
	rm -f *.a *.o *.so simd/*.o

.PHONY: clean
