all: sender receiver

sender: sender.o
	gcc -Wall -g -o sender sender.o

sender.o: sender.c
	gcc -Wall -g -c sender.c

receiver: receiver.o
	gcc -Wall -g -o receiver receiver.o

receiver.o: receiver.c
	gcc -Wall -g -c receiver.c

clean:
	rm sender receiver *.o received.txt

.PHONY: all clean