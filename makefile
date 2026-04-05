flags = -Wall -Werror 
debug_flags = -g3 -fsanitize=address -O1 -fno-omit-frame-pointer

all: test

libthreadpool.so : threadpool.c libqueue.so
	gcc threadpool.c $(flags) -fPIC -shared -L. -lqueue -o libthreadpool.so

test : libthreadpool.so test.c
	gcc test.c ${flags} -o test -L. -lthreadpool -o test
 
clean:
	rm *test