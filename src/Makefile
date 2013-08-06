tokenizer:
	clang++ -std=c++11  -g -O0 -c ./tokenizer/Tokenizer.cpp -o tokenizer.o #-DDEBUG
parser:	
	clang++ -std=c++11  -g -O0 -c ./parser/Parser.cpp -o parser.o #-DDEBUG
test:
	clang++ -std=c++11  -g -O0 -c ./parser/test.cpp -o main.o #-DDEBUG
ctx:	
	clang++ -std=c++11  -g -O0 -c ./runtime/Context.cpp -o context.o #-DDEBUG
main:	
	clang++ main.o parser.o tokenizer.o context.o -o a.out
clean:
	rm ./*.o -v -f ./*/*.o
build:	tokenizer parser test ctx