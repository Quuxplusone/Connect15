connect15: ab-timed.cpp main.cpp *.h
	$(CXX) -g -std=c++14 -O2 main.cpp ab-timed.cpp -o $@

tests: ab-timed.cpp main-tests.cpp *.h
	$(CXX) -g -std=c++14 -O2 main-tests.cpp ab-timed.cpp -o $@

test: tests
	./tests
