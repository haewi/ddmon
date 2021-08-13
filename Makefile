all:
	gcc -g -o test1 test1.c -pthread
	gcc -o test2 test2.c -pthread
	gcc -o test3 test3.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl
	gcc -o ddchck ddchck.c


clean:
	rm test1 test2 test3 ddmon.so ddchck .ddtrace
