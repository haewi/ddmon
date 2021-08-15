all:
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl
	gcc -o ddchck ddchck.c

one:
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread
	gcc -shared -fPIC -o one_ddmon.so one_ddmon.c -ldl

clean:
	rm test1 test2 test3 ddmon.so ddchck .ddtrace one_ddmon.so
