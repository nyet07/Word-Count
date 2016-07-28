/*
 ============================================================================
 Name        : client.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "sharedmem_def_client.h"

void input_tests(char **, int);
void generate_request(Request *, int, char **, char *, char *, char *);
char * concat(char *, char *);
void send_request(Request *, int, sem_t *, sem_t *);
void display_res(char *, int);

int is_empty(char * addr, char * key, sem_t * a, sem_t * b);
int read_part_no(char * addr, sem_t * a, sem_t * b);
int read_count(char * addr, sem_t * a, sem_t * b);
void read_part(char ** word, char * addr, int size, int offset, sem_t * a, sem_t * b);
int isFinished(char * addr, char * key, sem_t * a, sem_t * b);
int read_number_aux(char * addr, sem_t * a, sem_t *b);

void lock_blocking(int fd);
void lock_unlock(int fd);
void have_response(int, sem_t *, sem_t *);

int main(int argc, char * argv []) {

	//First test inputs

	input_tests(argv, argc);

	//All needed declarations

	int request_mem;
	int response_mem;

	//First have user-name

	char posix_prefix[POSIX_BUF] = {0};
	char * user_name = getenv("USER");
	posix_prefix[0] = '/';
	strcpy(posix_prefix + 1, user_name);

	//Create necessary names: reponse and request shm and semaphore A and B

	char * request_shm = concat(posix_prefix, REQUEST_SMEM); //Name to be used

	char * response_shm_template = concat(posix_prefix, CLIENT_SMEM_TEMPLATE);
	char response_shm [POSIX_BUF] = {0}; //Name to be used
	snprintf(response_shm, POSIX_BUF, response_shm_template, (long)getpid());

	//Semaphores for managing REQUEST order
	char * sem_A = concat(posix_prefix, SEMAP_A);//Name to be used
	char * sem_B = concat(posix_prefix, SEMAP_B);//Name to be used

	//Client specific semaphore_a to syncnorize for RESPONSE
	char * client_sem_template_a = concat(posix_prefix, CLIENT_SMEM_SEMAP_TEMPLATE_A);
	char client_sem_a [POSIX_BUF] = {0};
	snprintf(client_sem_a, POSIX_BUF, client_sem_template_a, (long)getpid());
	free(client_sem_template_a);

	//Client specific semaphore_b to syncnorize for RESPONSE
	char * client_sem_template_b = concat(posix_prefix, CLIENT_SMEM_SEMAP_TEMPLATE_B);
	char client_sem_b [POSIX_BUF] = {0};
	snprintf(client_sem_b, POSIX_BUF, client_sem_template_b, (long)getpid());
	free(client_sem_template_b);

	//Create or open necessary semaphores for REQUEST
	sem_t * sem_a = NULL, * sem_b = NULL;

	sem_a = sem_open(sem_A, O_RDWR | O_CREAT, 0777, 0);
	if (sem_a == SEM_FAILED){
		perror("sem_A_open");
		exit(EXIT_FAILURE);
	}

	sem_b = sem_open(sem_B, O_RDWR | O_CREAT, 0777, 0);
	if (sem_b == SEM_FAILED){
		perror("sem_B_open");
		exit(EXIT_FAILURE);
	}

	//Create necessary semaphores for RESPONSE
	sem_t * sem_client_a = NULL, * sem_client_b = NULL;

	sem_client_a = sem_open(client_sem_a, O_RDWR | O_CREAT, 0777, 0);
	if (sem_client_a == SEM_FAILED){
		perror("sem_client_error");
		exit(EXIT_FAILURE);
	}

	sem_client_b = sem_open(client_sem_b, O_RDWR | O_CREAT, 0777, 0);
	if (sem_client_b == SEM_FAILED){
		perror("sem_client_error");
		exit(EXIT_FAILURE);
	}

	//Get a file descriptor to request shared memory
	if((request_mem = shm_open(request_shm, O_RDWR, 0777)) < 0){
		perror("Server Shared Memory open error");
		exit(EXIT_FAILURE);
	}

	//Create shared memory for response
	if((response_mem = shm_open(response_shm, O_RDWR | O_CREAT , 0777)) < 0){
		perror("Shared Memory Creation Error");
		exit(EXIT_FAILURE);
	}

	//Resize response shared memory to be four kilobyte at most
	if(ftruncate(response_mem, FOUR_KIB) < 0){
		perror("Error Setting Size of Shared Memory");
		exit(EXIT_FAILURE);
	}

	//Declare necessary structure to cover requests
	Request req;

	//Fill a request structure
	generate_request(&req, argc ,argv, response_shm ,client_sem_a, client_sem_b);

	//Send the entire request to shared_mem
	send_request(&req, request_mem, sem_a, sem_b);

	/*	From this point on, responses are handled	*/

	//Also displays the result
	//have semaphores for sending having responses
	//Also displays the result within the function
	have_response(response_mem, sem_client_a, sem_client_b);

	//Delete client specific IPC
	shm_unlink(response_shm);
	sem_unlink(client_sem_a);
	sem_unlink(client_sem_b);

	//Free allocated space for names
	free(request_shm);
	free(response_shm_template);

	free(sem_A);
	free(sem_B);

	return 0;
}

void input_tests(char ** argv, int argc){

	char * dir_name = argv[1];      //Dir_name
	char * freq = argv[2];			//Frequency

	//Input if enough arguments are given.
	if (argc < 4) {
		fprintf(stderr, "At least 4 command-line arguments must be provided to this program:\n");
		fprintf(stderr, "1-)Working directory path of the server process\n");
		fprintf(stderr, "2-)Number of most frequent words to be printed\n");
		fprintf(stderr, "3-)A file containing the words not to be printed\n");
		fprintf(stderr, "4-)At least one file containing the words to be printed\n");
		fprintf(stderr, "Exiting... Please try again\n");
		exit(1);
	}

	//Check if the given string is a existing directory.
	struct stat sb;
	if(!(stat(dir_name, &sb) == 0 && S_ISDIR(sb.st_mode))){
		fprintf(stderr, "Directory location doesn't exist\n");
		exit(1);
	}

	for (unsigned int i = 0; i < strlen(freq); ++i) {
		if (freq[i] == '-') {
			fprintf(stderr, "Frequency must be a non-negative number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}

		else if (!isdigit(freq[i])) {
			fprintf(stderr, "Frequency must be a number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}
	}
}

void generate_request(Request * st, int ARGC ,char ** argv, char * response_shmem, char * semap_a, char * semap_b){

	//Integer
	st->word_freq = atoi(argv[2]);

	//Char Array
	strcpy(st->stop_words, argv[3]);

	//Copy name of client specific shared mem
	strcpy(st->res_shmem, response_shmem);

	//Copy client specific sem a
	strcpy(st->semap_a, semap_a);

	//Copy client specific sem b
	strcpy(st->semap_b, semap_b);

	const int FIX_ARGS = 4;
	const int FILE_COUNT = ARGC - FIX_ARGS;

	//Number of files to be read
	st->file_count = FILE_COUNT;

	//Temporary pointer to the array holding file names to be read
	char * tmp_ptr = st->files;

	//Handle non-static number of file names.
	for(int i = 0, file_no = i + 4; i < FILE_COUNT; ++i, ++file_no){
		const int LEN =  strlen(argv[file_no]) + 1;
		memcpy(tmp_ptr, argv[file_no], LEN);
		tmp_ptr += LEN;
	}
}

char * concat(char * s1, char * s2){
	char *result = calloc(strlen(s1)+strlen(s2)+1, sizeof(char));//+1 for the zero-terminator
	//in real code you would check for errors in malloc here
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

void send_request(Request * req, int fd, sem_t * sem_a, sem_t * sem_b){
	//Synranozation should happen right here.

	//Declare address of shared mem
	char * addr = NULL;

	//Lock, this will block if another process has lock on the file.
	lock_blocking(fd);

	//Write
	addr = mmap(NULL, FOUR_KIB, PROT_WRITE, MAP_SHARED, fd, 0);
	//fprintf(stderr, "mmap addr request: %p\n", addr);
	if(addr == MAP_FAILED)
		perror("mmap");
	close(fd);

	memcpy(addr, req, sizeof(Request));

	//Signal A, notifies server about request has been written
	sem_post(sem_a);

	//Wait B, this waits for server to notify client as to the server read response.
	sem_wait(sem_b);

	//Now request shared mem can be Unlocked
	lock_unlock(fd);

	//Delete mapping
	munmap(addr, FOUR_KIB);

}

void have_response(int fd, sem_t * sem_a, sem_t * sem_b){

	//Protocol first sends the count then sends the string

	//Address for shared memory used for delivering response
	char * addr = NULL;

	//Word pointer later to be used
	char * word = NULL;

	//Key to check against
	char key_empty[FOUR_KIB] = {0};
	memset(key_empty, '\n', FOUR_KIB - 1);

	//Read from shared
	addr = mmap(NULL, FOUR_KIB, PROT_READ, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED){
		fprintf(stderr, "Problem with mmap during response %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);

	if(addr == MAP_FAILED)
		perror("mmap");

	//Call is empty
	if(is_empty(addr, key_empty, sem_a, sem_b)){
		return;
	}

	//Have response start after here
	while(1){

		//initialization
		int i = 0;
		int size = FOUR_KIB;
		int offset = 0;
		int part = read_part_no(addr, sem_a, sem_b);

		while(i < part){
			read_part(&word, addr, size, offset, sem_a, sem_b);
			size += FOUR_KIB;
			offset += FOUR_KIB;
			++i;
		}

		//Now read count
		int count = read_count(addr, sem_a, sem_b);

		display_res(word, count);
		{
			free(word);
			word = NULL;
		}
		if(isFinished(addr, key_empty ,sem_a, sem_b)) break;
	}
}

int is_empty(char * addr, char * key, sem_t * a, sem_t * b){
	char buf [FOUR_KIB];

	sem_wait(a);
	memcpy(buf, addr, FOUR_KIB);
	sem_post(b);

	if(strcmp(addr, key) == 0){
		return 1;
	}
	return 0;
}

int read_part_no(char * addr, sem_t * a, sem_t * b){
	return read_number_aux(addr, a, b);
}

int read_count(char * addr, sem_t * a, sem_t * b){
	return read_number_aux(addr, a, b);
}

void read_part(char ** word, char * addr, int size, int offset, sem_t * a, sem_t * b){
	*word = (char *)realloc(*word, size);
	sem_wait(a);
	memcpy((*word + offset), addr, FOUR_KIB);
	sem_post(b);
}

int read_number_aux(char * addr, sem_t * a, sem_t *b){
	int number = 0;
	sem_wait(a);
	memcpy(&number, addr, sizeof(int));
	sem_post(b);
	return number;
}

int isFinished(char * addr, char * key, sem_t * a, sem_t * b){
	char buf [FOUR_KIB];

	sem_wait(a);
	memcpy(buf, addr, FOUR_KIB);
	sem_post(b);

	//if finished return 1
	if(strcmp(key, buf) == 0){
		return 1;
	}else{
		return 0;
	}
}

void display_res(char * word, int count){
	printf("%s %d\n", word, count);
}

void lock_blocking(int fd){

    struct flock fl;

	fl.l_type= F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	if(fcntl(fd, F_SETLKW, &fl) < 0){
		fprintf(stderr, "Signal is caught, program exiting...\n");
		exit(EXIT_FAILURE);
	}
}

void lock_unlock(int fd){
	struct flock fl;

	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fcntl(fd, F_SETLKW, &fl);
}

