CXX = g++-11
CXXFLAGS = -std=c++20 -Wno-attributes
CPPFLAGS = -g -I/usr/local/lib/python3.9/site-packages/numpy/core/include -I/usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/include/python3.9
LDFLAGS = -g /usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/Python

EXE = main

main: main.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) main.o -o $(EXE)

main.o: main.cpp ising_model.hpp repl.hpp spin.hpp external-libraries/matplotlibcpp.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp

.PHONY : clean
clean:
	rm -f *.o $(EXE)
