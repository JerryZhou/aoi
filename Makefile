CXX = gcc -g -std=gnu++0x
CC = gcc -g -std=c89
AR = ar
CFLAGS = -c -O3 -Wall -fPIC

LIBS = -lstdc++

APP = aoi
SLIB = libaoi.a
OBJS = main.o aoi.o

GET_DEPENDS=$(shell gcc -MM $(1) | cut -d ':' -f 2)

all : $(APP)

$(APP):$(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

$(SLIB) : aoi.o
	$(AR) crs $@ $^

main.o : $(call GET_DEPENDS, main.cpp)
	$(CXX) -o $@ -c main.cpp

aoi.o : $(call GET_DEPENDS, aoi.c)
	$(CC) -o $@ -c aoi.c

output: 
	@echo $(LDFLAGS) 
	@echo $(LIBS)
	@echo $(OBJS)


.PHONY : all clean	

clean:
	-rm -f $(APP) $(OBJS)

