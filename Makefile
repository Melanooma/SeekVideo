
INCFLAG		+= -I./include
DEBUGFLAG	= -g -O0 -Wall
CFLAGS		+= $(DEBUGFLAG) $(INCFLAG) -fpermissive
LDFLAGS	  += -lseekware -lavcodec  -lavutil -lswscale
CXXFLAGS	+= $(CFLAGS)
CPPFLAGS	+= $(CFLAGS)
CXX				= gcc

TARGET	= seekware-video
OBJECTS	= objs/seekware-video.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDPATH) $(LDFLAGS)

objs/%.o:	src/%.cpp
	@if test ! -e objs; then \
		mkdir objs  ;\
	fi;
	$(CXX) $(CXXFLAGS) -c $< -o $@

objs/%.o:	src/%.c
	@if test ! -e objs; then \
		mkdir objs  ;\
	fi;
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf core *~  $(TARGET) $(OBJECTS) $(LIB) $(LIBOBJ) src/*~ include/*~
