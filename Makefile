CXX = g++
CXXFLAGS = -Wall -O3 -march=native -s -std=c++11
INSTALL_PROGRAM = install
BINDIR = /usr/bin

all: doubleauction

clean:
	rm doubleauction

doubleauction: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o doubleauction

install: doubleauction
	$(INSTALL_PROGRAM) doubleauction $(BINDIR)/doubleauction
