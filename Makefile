all:
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread
	gcc -g -o test4 test4.c -pthread
	gcc -g -o test5 test5.c -pthread
	gcc -g -o test6 test6.c -pthread
	gcc -g -o test7 test7.c -pthread
	gcc -g -o test8 test8.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -ldl
	gcc -o ddchck ddchck.c -pthread

one:
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread
	gcc -g -o test4 test4.c -pthread
	gcc -g -o test5 test5.c -pthread
	gcc -g -o test6 test6.c -pthread
	gcc -g -o test7 test7.c -pthread
	gcc -g -o test8 test8.c -pthread
	gcc -shared -fPIC -o one_ddmon.so one_ddmon.c -ldl

pred:
	gcc -g -o test1 test1.c -pthread
	gcc -g -o test2 test2.c -pthread
	gcc -g -o test3 test3.c -pthread
	gcc -g -o test4 test4.c -pthread
	gcc -g -o test5 test5.c -pthread
	gcc -g -o test6 test6.c -pthread
	gcc -g -o test7 test7.c -pthread
	gcc -g -o test8 test8.c -pthread
	gcc -shared -fPIC -o pred_ddmon.so pred_ddmon.c -ldl
	gcc -o ddpred ddpred.c -pthread

clean:
	rm test1 test2 test3 test4 test5 test6 test7 test8 ddmon.so ddchck .ddtrace one_ddmon.so pred_ddmon.so ddpred  k_ddmon.so k_ddpred
