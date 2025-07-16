CXX=g++
CXXFLAGS=-std=c++17 -I/usr/include/postgresql
LDFLAGS=-lpq
OBJS=src/main.o src/riot_api.o src/db.o src/summary.o

all: main

main: $(OBJS)
	$(CXX) $(CXXFLAGS) -o main $(OBJS) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main
