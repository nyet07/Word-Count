PROJECT = prj3

TARGET = word-countd word-count

#Files needed for client and server.  List as per your organization.
#Should have a .h file which contains information common to both 
#client and server.
PROG_FILES = client.c server.c word-store.c fifo_def_client.h fifo_def_server.h word-store.h

#All files to be submitted
SRC_FILES = \
  $(PROG_FILES) \
  Makefile \
  README

#List name of all objects needed for building client.
CLIENT_OBJS = client.o 


#List name of all objects needed for building server.
SERVER_OBJS = server.o word-store.o

CC = gcc
CFLAGS = -g -Wall -std=gnu11 
LDFLAGS= -ldl


all:		$(TARGET) $(MODULES)

word-count: 	$(CLIENT_OBJS)
		$(CC) $(CLIENT_OBJS) $(CDFLAGS) -o $@

word-countd: 	$(SERVER_OBJS)
		$(CC) $(SERVER_OBJS) $(CDFLAGS) -o $@

#Removes all objects and executables.
clean:
		rm -f $(CLIENT_OBJS) $(SERVER_OBJS) $(TARGET) *.mod test *~

#This target makes a archive containing all the project source files,
#suitable for submission.
submit:		$(SRC_FILES)
		tar -cvzf $(PROJECT).tar.gz $(SRC_FILES)

#This target outputs dependencies of all C files on stdout.  The output can
#then be cut-and-pasted into this Makefile.
DEPEND:
		$(CC) $(CPPFLAGS) -MM *.c

#Automatically produced dependencies produced by 'make DEPEND'.
alnum-quote.o: alnum-quote.c word-char.h
main.o: main.c
modules-test.o: modules-test.c word-char.h
non-space.o: non-space.c word-char.h
