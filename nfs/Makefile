all: ./bin/jobCommander ./bin/jobExecutorServer

./bin/jobCommander: ./src/jobCommander.c 
	gcc -o ./bin/jobCommander ./src/jobCommander.c -Wall -Wextra -std=c99 -pthread -Iinclude
 
./bin/jobExecutorServer: ./src/jobExecutorServer.c ./bin/queue.o
	gcc -o ./bin/jobExecutorServer src/jobExecutorServer.c ./bin/queue.o -Wall -Wextra -std=c99 -pthread -Iinclude

./bin/queue.o: src/queue.c include/queue.h
	gcc -c -o ./bin/queue.o ./src/queue.c -Wall -Wextra -std=c99 -Iinclude