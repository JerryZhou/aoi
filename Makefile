
.PHONY:all
all:aoi

output: 
	@echo $(LDFLAGS) 
	@echo $(LIBS)

objs= aoi.o main.o

main.o:aoi.h aoitest.h main.cpp

aoi.o:aoi.h aoi.c

aoi:$(objs)
	cc -o aoi $(objs) $(LIBS)

.PHONY:clean	
clean:
	rm $(all) $(objs)

