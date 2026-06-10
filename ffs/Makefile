all: fss_console fss_manager worker

fss_console: src/fss_console.c
		gcc -Wall -o fss_console src/fss_console.c

fss_manager: src/fss_manager.c src/sync_info_mem_store.c src/sync_info_mem_store.h
		gcc -Wall -o fss_manager src/fss_manager.c src/sync_info_mem_store.c

worker: src/worker.c
		gcc -Wall -o worker src/worker.c

clean:
	rm -f fss_manager fss_console worker *.o *.log fss_in fss_out