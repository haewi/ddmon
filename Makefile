all:
	gcc -o test1 test1.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl


clean:
	rm test1 ddmon.so
