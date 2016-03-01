CXX = gcc -g -std=c++0x
CFLAGS = -c -O3 -Wall -fPIC 

CXXLDLIBS = -lstdc++

APP = aoi

all : $(APP) 

# draw app
$(APP):AOIPlugin.o draw.o main.o ../libaoi.a
	$(CXX) -o $@ $^ $(CXXLDLIBS) -I ../ -L ../ -laoi -framework OpenGL -framework GLUT

AOIPlugin.o: AOIPLugin.cpp ../aoi.o
	$(CXX) -o $@ $(CFLAGS) AOIPLugin.cpp -I ../ -L ../

draw.o: draw.cpp draw.h
	$(CXX) -o $@ $(CFLAGS) draw.cpp

main.o: main.cpp draw.o AOIPlugin.o
	$(CXX) -o $@ $(CFLAGS) main.cpp -I ../ -L ../

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) $*.c > $*.d
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
	-rm -f $(APP) *.o *.d
	

