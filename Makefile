


test_chen:
	g++ thread_chen.cc test.cc -g -Wall libcpu.o -ldl -pthread -std=c++11 -o test.out
	./test.out

test:
	g++ thread.cc test.cc -g -Wall libcpu.o -ldl -pthread -std=c++11 -o test.out
	./test.out


signal:
	g++ signal.cc -std=c++11 -o signal.out
	./signal.out


ceshi:
	g++ ceshi.cc -std=c++11 -o ceshi.out 
	./ceshi.out

thread:
	g++ thread.cc -o thread.out

main:
	g++ thread.cc app.cc libcpu.o -ldl -pthread -std=c++11 -o app.out

clean:
	rm -f *.out



git:
	git add .
	git commit -m 'auto'
	git push


re:
	make clean
	make ceshi	
