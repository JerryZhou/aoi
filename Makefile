CXX = gcc -g -std=c++0x
CC = gcc -g -std=c89
AR = ar
CFLAGS = -c -O3 -Wall -fPIC
PLATFORM = $(shell uname)

CXXLDLIBS = -lstdc++

APP = aoi


# for c
STATIC_LIB = libaoi.a

# for lua
LUA_SHARED_LIB = laoi.so
LUA_STATIC_LIB = laoi.a

ALL_LIBS = $(STATIC_LIB)

ENABLE_LUALIB=

LUA_LDFLAGS = 

ifeq ($(PLATFORM),Linux)
else
        ifeq ($(PLATFORM), Darwin)
                LUA_LDFLAGS += -llua
        endif
endif

ifeq ($(ENABLE_LUALIB), true)
	ALL_LIBS += $(LUA_SHARED_LIB) $(LUA_STATIC_LIB)
endif

OBJS = main.o aoi.o

GET_DEPENDS=$(shell gcc -MM $(1) | cut -d ':' -f 2)

all : $(APP) $(ALL_LIBS)

$(APP):$(OBJS)
	$(CXX) -o $@ $^ $(CXXLDLIBS)

$(STATIC_LIB) : aoi.o
	$(AR) crs $@ $^

$(LUA_STATIC_LIB) : aoi.o laoi.o
	$(AR) crs $@ $^

$(LUA_SHARED_LIB) : aoi.o laoi.o
	$(CC) -o $@ $^ --shared $(LUA_LDFLAGS)

laoi.o : $(call GET_DEPENDS, laoi.c)
	$(CC) -o $@ $(CFLAGS) laoi.c

main.o : $(call GET_DEPENDS, main.cpp)
	$(CXX) -o $@ $(CFLAGS) main.cpp

aoi.o : $(call GET_DEPENDS, aoi.c)
	$(CC) -o $@ $(CFLAGS) aoi.c

output: 
	@echo $(LDFLAGS) 
	@echo $(CXXLDLIBS)
	@echo $(OBJS)

.PHONY : all clean	

clean:
	-rm -f $(APP) $(OBJS) $(LUA_STATIC_LIB) $(LUA_SHARED_LIB) $(STATIC_LIB) laoi.o
	

