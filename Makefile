connect15: ab.cpp ab.h connect15.h main.cpp
	$(CXX) -std=c++14 -O2 main.cpp ab.cpp -o $@
