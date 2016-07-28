/*
 * fifo_def.h
 *
 *  Created on: Mar 24, 2016
 *      Author: ugur
 */

#ifndef SHAREDMEM_DEF_CLIENT_H_
#define SHAREDMEM_DEF_CLIENT_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define REQUEST_SMEM "_request_shm"
#define CLIENT_SMEM_TEMPLATE "_response_shm%ld"

#define SEMAP_A "_request_sem_a"
#define SEMAP_B "_request_sem_b"
#define CLIENT_SMEM_SEMAP_TEMPLATE_A "_response_sem_a_%ld"
#define CLIENT_SMEM_SEMAP_TEMPLATE_B "_response_sem_b_%ld"
#define PADDING 20

#define SIZE 1000
#define SIZE_RES SIZE * 4
#define FOUR_KIB 4096

#define POSIX_BUF 50

//int is four bytes
//pid_t four bytes

typedef struct request{

	int word_freq;
	char stop_words[SIZE];
	char files[SIZE];
	char res_shmem [SIZE];
	char semap_a [SIZE/2];
	char semap_b [SIZE/2];

	int file_count;

}Request;

typedef struct response{
	int count;
	char * res;
} Response;


#endif /* SHAREDMEM_DEF_CLIENT_H_ */
