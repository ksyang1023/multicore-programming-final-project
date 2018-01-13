CC = gcc
CFLAGS = -g -O3
LIBS = -fopenmp -lrt -lm -lpthread
SRCS = bfstest.c
TARGET = bfstest
all:
	gcc ${CFLAGS}  ${LIBS}  bfstest.c -o bfstest -lrt
pthread:
	gcc ${CFLAGS}  ${LIBS}  bfstest_pthread_v2.c -o bfstest_pthread_v2 -lrt
v2:
	gcc ${CFLAGS}  ${LIBS}  bfstest_v2.c -o bfstest_v2 -lrt
v3:
	gcc ${CFLAGS}  ${LIBS}  bfstest_v3.c -o bfstest_v3 -lrt
v4:
	gcc ${CFLAGS}  ${LIBS}  bfstest_v4.c -o bfstest_v4 -lrt
final_1:
	gcc ${CFLAGS}  ${LIBS}  final_1.c -o final_1 -lrt
final:
	gcc ${CFLAGS}  ${LIBS}  final.c -o final -lrt
clean:
	rm -rf ${TARGET}.out ${TARGET} 
