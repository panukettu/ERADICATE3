CC=g++
CDEFINES=
SOURCES=Dispatcher.cpp eradicate2.cpp hexadecimal.cpp ModeFactory.cpp Speed.cpp sha3.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ERADICATE2.x64
UNAME_S := $(shell uname -s)
ARCHOS := $(shell uname -sm | perl -pe 's/(.*?)\s(x)?(?:86_)?(.*?)$$/$$2$$3-\L$$1/; s/darwin/osx/;')
CXXFLAGS=-"I$(cwd)/vcpkg_installed/$(ARCHOS)/include"


ifeq ($(UNAME_S),Darwin)
	LDFLAGS+=-framework OpenCL
	CFLAGS+=-c -std=c++17 -Wall
else
	LDFLAGS+=-s -lOpenCL -mcmodel=large
	CFLAGS+=-c -std=c++17 -Wall -mmmx -O2 -mcmodel=large
endif

all: $(SOURCES) $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(CXXFLAGS) $(CDEFINES) $< -o $@

clean:
	rm -rf *.o