/*
 * fifo_def.h
 *
 *  Created on: Mar 24, 2016
 *      Author: ugur
 */

#ifndef FIFO_DEF_SERVER_H_
#define FIFO_DEF_SERVER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_FIFO "/request_fifo"
#define CLIENT_FIFO_TEMPLATE "/client_%ld"
#define PADDING 20
#define BUF_SIZE 100000

typedef struct request{

	int number_of_files;
	int freq;
	pid_t pid;

	char * word_c_module;
	char * stop_words;
	char ** files_to_read;

}Request;

typedef struct response{
	int count;
	char * word;
} Response;


#endif /* FIFO_DEF_SERVER_H_ */
