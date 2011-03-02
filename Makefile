PROG = CacheTestDriver
PINTOOL = MultiCacheSim_PinDriver.so

SRCS = SMPCache.cpp MultiCacheSim.cpp CacheCore.cpp Snippets.cpp nanassert.cpp
FACSRCS = MSI_SMPCache.cpp MESI_SMPCache.cpp

ifeq ($(BLDTYPE),pin)
PIN_KIT=$(PIN_HOME)
KIT=1
include $(PIN_HOME)/source/tools/makefile.gnu.config
CXXFLAGS += -DPIN
SRCS += MultiCacheSim_PinDriver.cpp
TARG = $(PINTOOL)
else
CXXFLAGS += -UPIN
LDFLAGS = -lpthread -ldl
SRCS += CacheTestDriver.cpp
TARG = $(PROG)
endif


CXXFLAGS += -I. -g -O0

OBJS = $(SRCS:%.cpp=%.o)
FACOBJS = $(FACSRCS:%.cpp=%.so)


all: $(TARG)

test: $(PROG)
	./CacheTestDriver

## build rules
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

%.so : %.cpp
	$(CXX) -fPIC -shared $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $< SMPCache.cpp Snippets.cpp nanassert.cpp

$(PROG): $(OBJS) 
	$(CXX) $(LDFLAGS) -o $@ $+ $(DBG)

$(PINTOOL): $(OBJS)
	$(CXX) $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $+ $(PIN_LIBS) $(DBG)

## cleaning
clean:
	-rm -f *.o $(PROG) $(PINTOOL)

-include *.d
