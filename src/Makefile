tokenizer:
	clang++ -std=c++11  -g -O0 -c ./tokenizer/Tokenizer.cpp -o tokenizer.o #-DDEBUG
parser:	
	clang++ -std=c++11  -g -O0 -c ./parser/Parser.cpp -o parser.o #-DDEBUG
test:
	clang++ -std=c++11  -g -O0 -c ./parser/test.cpp -o main.o #-DDEBUG
main:	
	clang++ main.o parser.o tokenizer.o -o a.out
clean:
	rm ./*.o -v -f ./*/*.o
build:	tokenizer parser test main