
.PHONY:all
all:aoi

output: 
	@echo $(LDFLAGS) 
	@echo $(LIBS)

objs= aoi.o main.o

aoi:$(objs)
	cc -o aoi $(objs) $(LIBS)

.PHONY:clean	
clean:
	rm $(all) $(objs)

