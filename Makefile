CC = gcc
CFLAGS = -Wall -g

biceps: biceps.o gescom.o
	$(CC) $(CFLAGS) -o biceps biceps.o gescom.o -lreadline

biceps.o: biceps.c gescom.h
	$(CC) $(CFLAGS) -c biceps.c

gescom.o: gescom.c gescom.h
	$(CC) $(CFLAGS) -c gescom.c

clean:
	rm -f biceps *.o
