/*
 * fifo_def.h
 *
 *  Created on: Mar 24, 2016
 *      Author: ugur
 */

#ifndef FIFO_DEF_CLIENT_H_
#define FIFO_DEF_CLIENT_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_FIFO "/request_fifo"
#define CLIENT_FIFO_TEMPLATE "/client_%ld"
#define PADDING 20
#define BUF_SIZE 100000

typedef struct request{
	char * word_c_module;
	char * most_freq_word;
	char * stop_words;
	char ** files_to_read;
	pid_t pid;
}Request;

typedef struct response{
	int count;
	char * word;
} Response;


#endif /* FIFO_DEF_CLIENT_H_ */
