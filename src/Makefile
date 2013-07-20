tokenizer:
	clang++ -std=c++11 -DDEBUG -g -O0 -c ./tokenizer/Tokenizer.cpp -o tokenizer.o 
parser:	
	clang++ -std=c++11 -DDEBUG -g -O0 -c ./parser/Parser.cpp -o parser.o
test:
	clang++ -std=c++11 -DDEBUG -g -O0 -c ./parser/test.cpp -o main.o
main:	
	clang++ main.o parser.o tokenizer.o -o a.out
clean:
	rm ./*.o -v -f ./*/*.o
build:	tokenizer parser test main