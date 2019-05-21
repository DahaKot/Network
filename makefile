target: starter.o worker.o net.o calculate.o
	gcc starter.o net.o -o starter
	gcc worker.o net.o calculate.o -o worker -pthread -O2

starter.o: starter.c
	gcc -c starter.c -o starter.o -D_GNU_SOURCE

worker.o: worker.c
	gcc -c worker.c -o worker.o 

net.o: net.c net.h
	gcc -c net.c -o net.o -D_GNU_SOURCE

calculate.o: calculate.c calculate.h
	gcc -c calculate.c -o calculate.o -pthread -O2

clean:
	rm starter worker
	rm *.o
-include *.d