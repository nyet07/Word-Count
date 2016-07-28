#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "sharedmem_def_server.h"
#include "word-store.h"

void input_test(char **);
void Lock();
int daemon_init(char *);
char * concat(char *, char *);

void read_request(int, char *, sem_t *, sem_t *);
Request return_request(int);
void partition_send(char *, char * , sem_t *, sem_t *);
void send_count(char *, int, sem_t *, sem_t *);
void send_results(Tree *, Tree *, int, char *, char *, char *);
void send_count(char *, int , sem_t * , sem_t * );
void indicate(char *, sem_t *, sem_t *);

void cleanUpMemory(Tree *, Tree *);

bool fileExists(const char *);

char posix_prefix[POSIX_BUF] = {0};

int main(int argc, char * argv []){

	//Check if the given directory exists
	input_test(argv);

	//If it does then start the daemon
	daemon_init(argv[1]);
}

void input_test(char ** argv){

	char * dir_name = argv[1];

	//Check if the given string is a existing directory.
	struct stat sb;
	if(!(stat(dir_name, &sb) == 0 && S_ISDIR(sb.st_mode))){
		fprintf(stderr, "Directory location doesn't exist\n");
		exit(EXIT_FAILURE);
	}
}

void Lock(char * dir_name){

	char * FILENAME = "/.pid";

	//Open .pid file first
	int fd_pid = -1;
	//Check the operation to open he file is successful or not

	char * absolute_path = concat(dir_name, FILENAME);
	if((fd_pid = open(absolute_path, O_WRONLY | O_CREAT, 0777)) < 0){
		fprintf(stderr, "There is error opening the .pid file\n");
		exit(EXIT_FAILURE);
	}
	free(absolute_path);

	//Declare the struct for file locking
	struct flock fl = {
		F_WRLCK,
		SEEK_SET,
		0,
		0,
	};

	//If file is locked by another process print already working process and exit this process
	//Or if a lock error happens just exit the program
	if(fcntl(fd_pid, F_SETLK, &fl) < 0){
		if (errno == EAGAIN && fcntl(fd_pid, F_GETLK, &fl) >= 0) {
			fprintf(stderr, "process %ld has lock\n", (long)fl.l_pid);
		}
		else {
			fprintf(stderr, "lock error: %s\n", strerror(errno));
		}
		exit(EXIT_FAILURE);
	} //Else

	//Truccate the file to clean its contents
	if(ftruncate(fd_pid, 0) != 0){
		fprintf(stderr, "cannot truncate %s: %s\n", FILENAME, strerror(errno));
		exit(EXIT_FAILURE);
	}

	FILE * f = fdopen(fd_pid, "w");
	if(!f){
		fprintf(stderr, "cannot fdopen %s(%d): %s\n", FILENAME, fd_pid, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf(f, "%ld\n", (long)getpid());

	//This makes sure that pid is written inside .pid

	fflush(f);
}

int daemon_init(char * dir_name){

	pid_t pid, sid;
	if((pid = fork()) < 0){
		perror("Fork error");
		exit(EXIT_FAILURE);
	}
	else if(pid > 0){
		printf("PID of deamon: %ld\n", (long)pid);
		exit(EXIT_SUCCESS);
	}
	else{

		//Child becomes deamon
		umask(0);

		if((sid = setsid() < 0)){
			exit(EXIT_FAILURE);
		}

		if((chdir(dir_name)) < 0){
			exit(EXIT_FAILURE);
		}

		//Try to get exclusive lock on text file .pid in DIR_NAME
		//Exits if it can't lock the .pid file
		Lock(dir_name);

		//Just close the STDIN but leave rest open for debug purposes (later to be removed)
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		//Close all output lines
		fclose(stdout);
		fclose(stdin);
		fclose(stderr);

		//Obtain user name and create prefix mentioned in the assignment
		//Posix prefix is global variable
		char * user_name = getenv("USER");
		posix_prefix[0] = '/';
		strcpy(posix_prefix + 1, user_name);

		//Create request name for request shared memory
		char * request_shm = concat(posix_prefix, REQUEST_SMEM); //Name to be used
		char request_smem[POSIX_BUF] = {0};
		strcpy(request_smem, request_shm);
		free(request_shm);

		//Generate semaphore_A name
		char * sem_A = concat(posix_prefix, SEMAP_A);//Name to be used
		char static_sem_A[POSIX_BUF] = {0};
		strcpy(static_sem_A, sem_A);
		free(sem_A);

		//Open semaphore A
		sem_t * sem_a = NULL;
		sem_a = sem_open(static_sem_A, O_RDWR | O_CREAT, 0777, 0);
		if (sem_a == SEM_FAILED){
			perror("sem_A_open");
			exit(EXIT_FAILURE);
		}

		//Generate semaphore_B name
		char * sem_B = concat(posix_prefix, SEMAP_B);//Name to be used
		char static_sem_B[POSIX_BUF] = {0};
		strcpy(static_sem_B, sem_B);
		free(sem_B);

		//Open semaphore B
		sem_t * sem_b = NULL;
		sem_b = sem_open(static_sem_B, O_RDWR | O_CREAT, 0777, 0);
		if (sem_b == SEM_FAILED){
			perror("sem_B_open");
			exit(EXIT_FAILURE);
		}

		//Open the Shared Memory for Requests
		int request_mem_d = shm_open(request_smem, O_RDWR | O_CREAT, 0777);
		if(request_mem_d == -1){
			fprintf(stderr, "An error happened when opening Shared Memory\n");
			exit(EXIT_FAILURE);
		}

		//Resize shared memory object for request to be FOUR KILOBYTE
		if(ftruncate(request_mem_d, FOUR_KIB) < 0){
			fprintf(stderr, "Truncate Error\n");
			exit(EXIT_FAILURE);
		}

		while(1){
			read_request(request_mem_d, dir_name, sem_a, sem_b);
		}
	}
	exit(EXIT_SUCCESS);
}

void read_request(int fd, char * dir_name, sem_t * sem_a, sem_t * sem_b){

	//Declarations
	pid_t pid;
	int wstatus;

	//Wait for read until client signals
	sem_wait(sem_a);

	//Read the request from shared memory
	Request req = return_request(fd);

	//Signal for client to unlock request shared memory
	sem_post(sem_b);

	if((pid = fork()) < 0){
		exit(EXIT_FAILURE);
	}
	else if(pid == 0){
		if((pid = fork()) < 0){
			exit(EXIT_FAILURE);
		}
		else if(pid > 0){
			exit(EXIT_SUCCESS);
		}
		else{
			//Child Handler Process
			sleep(1);

			//Request struct has necessary information, using this open other files
			Tree igWords;

			//Hold words to be printed
			Tree normalWords;

			//Call necessary functions for program flow
			{
				binary_tree_ctor(&igWords);
				binary_tree_ctor(&normalWords);
			}

			read_words(req.stop_words, &igWords);

			int offset = 0;
			for(int i = 0; i < req.file_count; ++i){
				read_words((req.files + offset), &normalWords);
				offset += strlen(req.files + offset) + 1;
			}

			send_results(&normalWords, &igWords, req.word_freq, req.res_shmem, req.semap_a, req.semap_b);

			cleanUpMemory(&normalWords, &igWords);

			exit(EXIT_SUCCESS);
		}
	}
	//Code executed by the parent
	else{
		waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
	}
}

char * concat(char * s1, char * s2){
	char *result = calloc(strlen(s1)+strlen(s2)+1, sizeof(char));//+1 for the zero-terminator
	//in real code you would check for errors in malloc here
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

bool fileExists(const char * file){
	struct stat buf;
	return (stat(file, &buf) == 0);
}

Request return_request(int fd){
	Request req;
	char * addr = mmap(NULL, FOUR_KIB, PROT_READ, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED){
		perror("mmap server");
		exit(EXIT_FAILURE);
	}

	memcpy(&req, addr, sizeof(Request));
	munmap(addr, FOUR_KIB);
	return req;
}

void send_results(Tree * norm, Tree * ign, int freq, char * response_shm, char * client_sem_a, char * client_sem_b){
	Node * most_freq = NULL;
	Node * tmp = NULL;

	//Semaphores
	sem_t * sem_a = NULL, * sem_b = NULL;

	//Address for shared memory
	char * addr = NULL;

	//Open shared memory here and write to it
	//Change this to O_RDONLY and try it that way too
	int fd = shm_open(response_shm, O_RDWR, 0777);
	if(fd < 0){
		fprintf(stderr, "Error opening client_shm from server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//Map address and verify it
	addr = mmap(NULL, FOUR_KIB, PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED){
		fprintf(stderr, "Error happened during mmap in server: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	//fprintf(stderr, "mmap addr request: %p\n", addr);

	sem_a = sem_open(client_sem_a, O_RDWR, 0777, 0);
	if (sem_a == SEM_FAILED){
		perror("sem_client_error");
		exit(EXIT_FAILURE);
	}

	sem_b = sem_open(client_sem_b, O_RDWR, 0777, 0);
	if (sem_b == SEM_FAILED){
		perror("sem_client_error");
		exit(EXIT_FAILURE);
	}
	//If file is empty just do not do anything
	if(norm->root == NULL){
		//Set shared memory to be all zeros and check inside client
		memset(addr, 0, FOUR_KIB);
		memset(addr, '\n', FOUR_KIB - 1);

		sem_post(sem_a);
		sem_wait(sem_b);
		return;
	}

	//If ignorewords is not empty
	if(ign->root != NULL){
		for (int i = 0; i < freq; ++i) {
			do{
				tmp = most_freq;
				norm->most_occuring_word(norm, &most_freq);
				most_freq->visited = true;
				if (tmp == most_freq) break;

			} while(ign->is_present(ign, most_freq->word));

			if(most_freq != tmp){
				sem_post(sem_a);
				sem_wait(sem_b);

				partition_send(most_freq->word, addr, sem_a, sem_b);
				send_count(addr, most_freq->count ,sem_a, sem_b);
			}

			else{
				indicate(addr, sem_a, sem_b);
				break;
			}
			if(i == freq - 1){
				indicate(addr, sem_a, sem_b);
				break;
			}
		}
	}
	//If ign->root == NULL
	else{
		for (int i = 0; i < freq; ++i) {
			tmp = most_freq;
			norm->most_occuring_word(norm, &most_freq);
			most_freq->visited = true;
			if (tmp == most_freq) break;

			if(most_freq != tmp){
				sem_post(sem_a);
				sem_wait(sem_b);

				partition_send(most_freq->word, addr, sem_a, sem_b);
				send_count(addr, most_freq->count ,sem_a, sem_b);
			}
			else{
				indicate(addr, sem_a, sem_b);
				break;
			}
			if(i == freq - 1){
				indicate(addr, sem_a, sem_b);
				break;
			}
		}
	}
	munmap(addr, FOUR_KIB);
	close(fd);
}

void indicate(char * addr, sem_t * a, sem_t * b){
	memset(addr, '\n', FOUR_KIB - 1);
	sem_post(a);
	sem_wait(b);
}

void partition_send(char * word, char * addr, sem_t * semap_a, sem_t * semap_b){

	int size = strlen(word) + 1;
	const int TURN = (size / FOUR_KIB) + 1;
	char buf [FOUR_KIB] = {0};

	//First send number of transactions to send entire word
	memcpy(addr, &TURN, sizeof(const int));
	sem_post(semap_a);
	sem_wait(semap_b);

	//Partition the word and send it
	int offset = 0;
	for(int i = 0; i < TURN; ++i){
		//Copy part of word to buffer
		strncpy(buf, word + offset, FOUR_KIB);
		offset += FOUR_KIB;

		//Write to shared_memory here.
		memcpy(addr, buf, FOUR_KIB);
		memset(buf, 0, FOUR_KIB);

		sem_post(semap_a);
		sem_wait(semap_b);
	}
}

void send_count(char * addr, int count, sem_t * a, sem_t * b){
	//After send count
	memcpy(addr, &count, sizeof(int));
	sem_post(a);
	sem_wait(b);
}

void cleanUpMemory(Tree * norm, Tree * ign){
	norm->delete_tree(norm);
	ign->delete_tree(ign);
}
