
ifndef config
	config=debug
endif

COMPILE=g++
DEFINES=
NAME=demo
CPPFLAGS=-std=c++0x -MMD -MP -Wall -fno-math-errno -ffast-math -pthread -fno-exceptions -fno-rtti 

ifeq ($(config),release)
	DEFINES += -DRELEASE
	OBJDIR = obj/release
	TARGETDIR = .
	TARGET = $(TARGETDIR)/$(NAME)-z
	CPPFLAGS += -O3 $(DEFINES)
endif

ifeq ($(config),debug)
	DEFINES += -DDEBUG
	OBJDIR = obj/debug
	TARGETDIR = .
	TARGET = $(TARGETDIR)/$(NAME)-d
	CPPFLAGS += -g -ggdb $(DEFINES)
endif
	
#	INCLUDES =
LDFLAGS =
LIBS = -lGL -lGLU -lSDL -lGLEW -lrt

LINKCMD = $(COMPILE) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)

OBJECTS := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/vec.o \
	$(OBJDIR)/camera.o \
	$(OBJDIR)/commonmath.o \
	$(OBJDIR)/framemem.o \
	$(OBJDIR)/noise.o \
	$(OBJDIR)/tokparser.o \
	$(OBJDIR)/tweaker.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/menu.o \
	$(OBJDIR)/font.o \
	$(OBJDIR)/matrix.o \
	$(OBJDIR)/frame.o \
	$(OBJDIR)/debugdraw.o \
	$(OBJDIR)/task.o \
	$(OBJDIR)/gputask.o \
	$(OBJDIR)/poolmem.o \
	$(OBJDIR)/ui.o \

.PHONY: clean strip

all: $(TARGETDIR) $(OBJDIR) $(TARGET)
	@:

$(TARGET): $(OBJECTS)
	$(LINKCMD)

$(TARGETDIR):
	mkdir -p $(TARGETDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)

strip: $(TARGET)
	strip -s -R .comment -R .gnu.version $(TARGET)

$(OBJDIR)/main.o: main.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/vec.o: vec.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/camera.o: camera.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/commonmath.o: commonmath.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/framemem.o: framemem.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/noise.o: noise.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/tokparser.o: tokparser.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/tweaker.o: tweaker.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/render.o: render.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/menu.o: menu.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/font.o: font.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/matrix.o: matrix.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/frame.o: frame.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/debugdraw.o: debugdraw.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/task.o: task.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/gputask.o: gputask.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/poolmem.o: poolmem.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

$(OBJDIR)/ui.o: ui.cpp
	$(COMPILE) $(CPPFLAGS) -o "$@" -c "$<"

-include $(OBJECTS:%.o=%.d)

