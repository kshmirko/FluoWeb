CC=gcc
LDFLAGS=-lltrapi -lltr210api -lm

all: ltr210-fluo

%.o: %.c
	$(CC) -c -o $@ $<

ltr210-fluo: ltr210-fluo.o 
	$(CC) $< $(LDFLAGS) -o $@
	#ln -s ../ltr210/ltr210-fluo ../controller/ltr210-fluo

clear:
	rm -rf ltr210-fluo 
	#rm -rf ../controller/ltr210-fluo
	rm -rf *.o

