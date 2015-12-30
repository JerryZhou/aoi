CXX = gcc -g -std=c++0x
CC = gcc -g -std=c89
AR = ar
CFLAGS = -c -O3 -Wall -fPIC
PLATFORM = $(shell uname)

LIBS = -lstdc++

APP = aoi
STATIC_LIB = libaoi.a
LUA_SHARED_LIB = laoi.so
LUA_LDFLAGS = 

ifeq ($(PLATFORM),Linux)
else
        ifeq ($(PLATFORM), Darwin)
                LUA_LDFLAGS += -llua
        endif
endif

OBJS = main.o aoi.o

GET_DEPENDS=$(shell gcc -MM $(1) | cut -d ':' -f 2)

all : $(APP) $(LUA_SHARED_LIB)

$(APP):$(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

$(STATIC_LIB) : aoi.o
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
	@echo $(LIBS)
	@echo $(OBJS)

.PHONY : all clean	

clean:
	-rm -f $(APP) $(OBJS) $(LUA_SHARED_LIB) $(STATIC_LIB) laoi.o
	

