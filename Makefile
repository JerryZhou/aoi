CXX = gcc -g -std=c++0x
CC = gcc -g -std=c89
AR = ar
CFLAGS = -c -O3 -Wall -fPIC 
PLATFORM = $(shell uname)
PWD = $(shell pwd)

CXXLDLIBS = -lstdc++

APP = aoi

# for c
STATIC_LIB = libaoi.a

# for lua
LUA_SHARED_LIB = laoi.so
LUA_STATIC_LIB = laoi.a

ALL_LIBS = $(STATIC_LIB)

ENABLE_LUALIB = true

LUA_LDFLAGS = 

LUA_CFLAGS = -I /usr/local/include 

ifeq ($(PLATFORM),Linux)
else
        ifeq ($(PLATFORM), Darwin) 
			LUA_LDFLAGS += -llua -L /usr/local/lib 
        endif
endif

ifeq ($(ENABLE_LUALIB), true)
	ALL_LIBS += $(LUA_SHARED_LIB) $(LUA_STATIC_LIB)
endif

OBJS = main.o aoi.o

all : $(APP) $(ALL_LIBS)

$(APP):$(OBJS)
	$(CXX) -o $@ $^ $(CXXLDLIBS)

$(STATIC_LIB) : aoi.o
	$(AR) crs $@ $^

$(LUA_STATIC_LIB) : aoi.o laoi.o
	$(AR) crs $@ $^

$(LUA_SHARED_LIB) : aoi.o laoi.o
	$(CC) -o $@ $^ --shared $(LUA_LDFLAGS)

main.o : aoi.o aoitest.h
	$(CXX) -o $@ $(CFLAGS) main.cpp

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o $(LUA_CFLAGS)
	$(CC) -MM $(CFLAGS) $*.c > $*.d $(LUA_CFLAGS)
	@mv -f $*.d $*.d.tmp 
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

output: 
	@echo $(ALL_LIBS) 
	@echo $(LUA_LDFLAGS) 
	@echo $(LDFLAGS) 
	@echo $(CXXLDLIBS)
	@echo $(OBJS)

.PHONY : all clean	

clean:
	-rm -f $(APP) $(OBJS) $(LUA_STATIC_LIB) $(LUA_SHARED_LIB) $(STATIC_LIB) *.o *.d
	

