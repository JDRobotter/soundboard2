CC=clang++ -std=c++1y
CXXFLAGS= $(shell wx-config --cxxflags)

LD=clang++ -std=c++1y
LDFLAGS= $(shell wx-config --libs)

SOURCES= $(wildcard *.cpp)
HEADERS= $(wildcard *.hpp)

OBJS= $(SOURCES:.cpp=.o)

soundboard: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

.PHONY:
clean:
	rm -f *.o
