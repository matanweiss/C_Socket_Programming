all: Sender Receiver

Sender: Sender.o
	gcc -Wall -g -o Sender Sender.o

Sender.o: Sender.c
	gcc -Wall -g -c Sender.c

Receiver: Receiver.o
	gcc -Wall -g -o Receiver Receiver.o

Receiver.o: Receiver.c cvector.h
	gcc -Wall -g -c Receiver.c

clean:
	rm Sender Receiver *.o

.PHONY: all clean