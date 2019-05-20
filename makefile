target: starter worker

starter: starter.c
	gcc starter.c -o starter -pthread -O2 -D_GNU_SOURCE

worker: worker.c
	gcc worker.c -o worker -pthread -O2 -D_GNU_SOURCE

clean:
	rm ./a.out starter worker