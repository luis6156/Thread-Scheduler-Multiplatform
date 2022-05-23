COMPILER = gcc
LIBRARY_FLAG = -shared
FLAGS = -Wall -Wextra -g -fPIC

all: build

.PHONY: clean

build: so_scheduler.o priority_queue.o linkedlist.o hashtable.o
	$(COMPILER) $(LIBRARY_FLAG) $^ -o libscheduler.so

so_scheduler.o: so_scheduler.c
	$(COMPILER) $(FLAGS) -c $^

hashtable.o: hashtable.c
	$(COMPILER) $(FLAGS) -c $^

linkedlist.o: linkedlist.c
	$(COMPILER) $(FLAGS) -c $^

priority_queue.o: priority_queue.c
	$(COMPILER) $(FLAGS) -c $^	

clean:
	rm -rf *.o
	rm -f libscheduler.so
