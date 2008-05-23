CFLAGS=-W -Wall -pipe -O0 -g -DPACKAGE=libcanberra -pthread

test: proplist.o mutex-posix.o common.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o test
