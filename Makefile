CC=g++ -Wall -std=c++1y -O3 -g
CXXFLAGS= $(shell wx-config --cxxflags)

LD=g++ -Wall -std=c++1y -O3 -g
LDFLAGS= $(shell wx-config --libs) -lmad -lportaudio -lm -pthread -lsndfile

SOURCES= $(wildcard *.cpp)
HEADERS= $(wildcard *.hpp)

OBJS= $(SOURCES:.cpp=.o)

EXE=soundboard

$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<


.PHONY:
clean:
	rm -f $(OBJS) $(EXE)

