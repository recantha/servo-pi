CC	= gcc
AR	= ar
RANLIB  = ranlib
SIZE    = size

LIB	= libpigpio.a

CFLAGS	= -O2 -Wall

all:	checklib demolib

driver:		driver.o
	$(CC) -o driver driver.c -L/usr/local/lib -lpigpio -lpthread -lrt

checklib:	checklib.o
	$(CC) -o checklib checklib.c -L/usr/local/lib -lpigpio -lpthread -lrt

demolib:	demolib.o
	$(CC) -o demolib demolib.c -L/usr/local/lib -lpigpio -lpthread -lrt

$(LIB):	$(OBJ)
	$(AR) rcs $(LIB) $(OBJ)
	$(RANLIB) $(LIB)
	$(SIZE)   $(LIB)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f *.o *.i *.s *~ checklib demolib

install:	$(LIB) 
	sudo install -m 0755 -d          /usr/local/lib
	sudo install -m 0755 -d          /usr/local/include
	sudo install -m 0644 libpigpio.a /usr/local/lib
	sudo install -m 0644 pigpio.h    /usr/local/include

uninstall:
	rm -f /usr/local/lib/libpigpio.a
	rm -f /usr/local/include/pigpio.h

# DO NOT DELETE

checklib.o:	checklib.c libpigpio.a pigpio.h
demolib.o:	demolib.c libpigpio.a pigpio.h
