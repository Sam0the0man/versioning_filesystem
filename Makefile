.PHONY: run all clean

CXX = g++
CXXFLAGS = --std=c++17

INCLUDE_DIRS = -I/opt/homebrew/include
LDFLAGS += /opt/homebrew/lib
LIBS += -lcryptopp

TEST_DIR = default

objects = file.o diskLibrary.o

run: test
	./test

test: test.cpp $(objects)
	$(CXX) $(CXXFLAGS) $^ -o test

file.o: file.h file.cpp
	$(CXX) $(CXXFLAGS) -c file.cpp -o file.o

diskLibrary.o: diskLibrary.h file.h diskLibrary.cpp 
	$(CXX) $(CXXFLAGS) -c diskLibrary.cpp -o diskLibrary.o

clean:
	rm test
	rm -r $(TEST_DIR)	