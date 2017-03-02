CC=gcc
LDFLAGS=-lltrapi -lltr11api -lm

all: ltr11-fluo

%.o: %.c
	$(CC) -c -o $@ $<

ltr11-fluo: ltr11-fluo.o 
	$(CC) $< $(LDFLAGS) -o $@
	#ln -s ../ltr11/ltr11-fluo ../controller/ltr11-fluo

clear:
	rm -rf ltr11-fluo 
	#rm -rf ../controller/ltr11-fluo
	rm -rf *.o

