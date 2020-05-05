connect15: ab-timed.cpp ab-timed.h connect15.h main.cpp
	$(CXX) -std=c++14 -O2 main.cpp ab-timed.cpp -o $@
