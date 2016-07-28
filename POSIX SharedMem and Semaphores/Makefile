PROJECT = prj4

TARGET = word-countd word-count

#Files needed for client and server.  List as per your organization.
#Should have a .h file which contains information common to both 
#client and server.
PROG_FILES = sharedmem_def_server.h sharedmem_def_client.h word-store.h project4_server.c project4_client.c word-store.c 

#All files to be submitted
SRC_FILES = \
  $(PROG_FILES) \
  Makefile \
  README

#List name of all objects needed for building client.
CLIENT_OBJS = word-count.o

#List name of all objects needed for building server.
SERVER_OBJS = word-countd.o word-store.o

CC = gcc
CFLAGS = -g -Wall -std=c11 -c -D_XOPEN_SOURCE=500
LDFLAGS= -lrt -lpthread

all:	$(TARGET) $(MODULES)

word-count:		$(CLIENT_OBJS)
		$(CC) $(CLIENT_OBJS) $(LDFLAGS) -o $@

word-countd:	$(SERVER_OBJS)
		$(CC) $(SERVER_OBJS) $(LDFLAGS) -o $@

word-count.o: project4_client.c sharedmem_def_client.h
		$(CC) $(CFLAGS) project4_client.c -o $@

word-countd.o: project4_server.c sharedmem_def_server.h
		$(CC) $(CFLAGS) project4_server.c -o $@

word-store.o: word-store.c word-store.h
		$(CC) $(CFLAGS) word-store.c -o $@

clean:
		rm -f $(CLIENT_OBJS) $(SERVER_OBJS) $(TARGET) 

submit:		$(SRC_FILES)
		tar -cvzf $(PROJECT).tar.gz $(SRC_FILES)
