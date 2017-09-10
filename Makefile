CC=g++ -Wall -std=c++1y -O0 -g
CXXFLAGS= $(shell wx-config --cxxflags)

LD=g++ -Wall -std=c++1y -O0 -g
LDFLAGS= $(shell wx-config --libs) -lportaudio -lm -lmad -pthread -lsndfile

SOURCES= $(wildcard *.cpp)
HEADERS= $(wildcard *.hpp)

OBJS= $(SOURCES:.cpp=.o)

EXE=soundboard

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

_test:
	$(CC) $(CXXFLAGS) $(LDFLAGS) test/test.cpp audiomixer.cpp maddecoder.cpp wavdecoder.cpp -o _test

.PHONY:
clean:
	rm -f $(OBJS) $(EXE)

