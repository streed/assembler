GCC = g++

List.o: List.cpp List.h
	$(GCC) -c List.cpp

Parser.o: Parser.cpp Parser.h List.cpp List.h Utilities.h
	$(GCC) -c Parser.cpp

main.o: Parser.o main.cpp Parser.h
	$(GCC) -c main.cpp Parser.cpp

parser: Parser.o main.o
	$(GCC) -o parser main.cpp Parser.cpp

test: Parser.cpp Parser.h List.cpp List.h testMain.cpp testMain.h Utilities.h
	$(GCC) -D TESTING -o testing Parser.cpp List.cpp testMain.cpp main.cpp

clean:
	rm -rf *o parser
