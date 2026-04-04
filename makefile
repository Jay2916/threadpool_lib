flags = -Wall -Werror -Wno-deprecated-declarations
all: test

libthreadpool.so : threadpool.c
	gcc threadpool.c $(flags) -fPIC -shared -L. -lqueue -o libthreadpool.so

test : libthreadpool.so test.c
	gcc test.c  -o test -L. -lthreadpool -o test

chatgpt_test: chatgpt_test.c threadpool.c
	gcc chatgpt_test.c -L. -lthreadpool -lpthread -o chatgpt_test