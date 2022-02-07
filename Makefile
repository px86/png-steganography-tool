CXX = g++
CXXFLAGS = -Wall -Werror -Wpedantic -std=c++17 -Isrc/include
LIBS =

pngs: src/main.cpp pngs.o crc.o
	$(CXX) $(CXXFLAGS) $^ -o $@

pngs.o: src/pngs.cpp src/include/pngs.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

crc.o: src/crc.cpp src/include/crc.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: pngs example/image.png example/secret.txt
	./pngs example/image.png example/secret.txt output.png
	./pngs -x scrt output.png extracted-message.txt
	diff extracted-message.txt example/secret.txt

clean:
	rm -rf *.o pngs
