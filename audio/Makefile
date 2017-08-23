CC=clang++ -std=c++1y -g
CXXFLAGS= 

LD=clang++ -std=c++1y
LDFLAGS= -lportaudio -lm -lmad -pthread

SOURCES= $(wildcard *.cpp)
HEADERS= $(wildcard *.hpp)

OBJS= $(SOURCES:.cpp=.o)

audio: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

.PHONY:
clean:
	rm -f *.o
