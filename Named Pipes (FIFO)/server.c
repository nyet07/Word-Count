#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fifo_def_server.h"
#include "word-store.h"

int daemon_init(char *);
char * concat(char *, char *);
void generateServerFifoPath(char *, char *);
void generateClientFifoPath(char *, char *, int, pid_t);
void read_request(int, char *);
char * return_request(int);
void separate_inputs(Request *, char *);
void send_results(Tree *, Tree *, int, char *);

void cleanUpMemory(Tree *, Tree *);
void cleanUpReq(Request *);

bool fileExists(const char *);

int main(int argc, char * argv []){
	daemon_init(argv[1]);
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

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		const int SERVER_FIFO_NAME_LEN = strlen(dir_name) + strlen(SERVER_FIFO) + 1;
		char serverFifo[SERVER_FIFO_NAME_LEN];
		generateServerFifoPath(dir_name, serverFifo);

		if(!fileExists(serverFifo)){
			//Create the FIFO
			if(mkfifo(serverFifo, 0777) == -1){
				exit(EXIT_FAILURE);
			}
		}

		int request_fifo;
//		if((request_fifo = open(serverFifo, O_RDONLY)) < 0){
//			exit(EXIT_FAILURE);
//		}

		while(1){
			if((request_fifo = open(serverFifo, O_RDONLY)) < 0){
				exit(EXIT_FAILURE);
			}
			read_request(request_fifo, dir_name);
		}

	}
	exit(EXIT_SUCCESS);
}

char * concat(char * s1, char * s2){
	char *result = calloc(strlen(s1)+strlen(s2)+1, sizeof(char));//+1 for the zero-terminator
	//in real code you would check for errors in malloc here
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

void generateServerFifoPath(char * dir_name, char * serverFifo){
	char * fifo_place = concat(dir_name, SERVER_FIFO);
	strcpy(serverFifo, fifo_place);
	free(fifo_place);
}

void generateClientFifoPath(char * dir_name, char * clientFifo, int size, pid_t pid){
	char * client_place = concat(dir_name, CLIENT_FIFO_TEMPLATE);
	snprintf(clientFifo, size, client_place, (long) pid);
	free(client_place);
}

bool fileExists(const char * file){
	struct stat buf;
	return (stat(file, &buf) == 0);
}

void read_request(int fd, char * dir_name){
	pid_t pid;
	int wstatus;

	char * raw_req = return_request(fd);

	if((pid = fork()) < 0){
		exit(EXIT_FAILURE);
	}
	else if(pid == 0){
		if((pid = fork()) < 0){
			exit(EXIT_FAILURE);
		}
		else if(pid > 0){
			free(raw_req);
			exit(EXIT_SUCCESS);
		}
		else{
			//Child Handler Process
			sleep(1);
			Request req;

			///BURAYI TEKRAR YAZ
			separate_inputs(&req, raw_req);

			//Generate the name to access to the client FIFO
			const int CLIENT_FIFO_NAME_LEN = strlen(dir_name) + strlen(CLIENT_FIFO_TEMPLATE) + PADDING;
			char clientFifo[CLIENT_FIFO_NAME_LEN];
			generateClientFifoPath(dir_name, clientFifo, CLIENT_FIFO_NAME_LEN, req.pid);

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

			for(int i = 0; i < req.number_of_files; ++i){
				read_words(req.files_to_read[i], &normalWords);
			}

			send_results(&normalWords, &igWords, req.freq, clientFifo);

			cleanUpReq(&req);
			cleanUpMemory(&normalWords, &igWords);
			free(raw_req);
		}
	}
	else{
		waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
	}

	free(raw_req);
}

char * return_request(int fd){

	FILE * fp = fdopen(fd, "r");
	int c = -1;

	char * word = NULL;
	int initialSize = 20;
	int index = 0;

	if((word = (char *)calloc(initialSize, sizeof(char))) == NULL){
		exit(EXIT_FAILURE);
	}

	while((c = fgetc(fp)) != EOF){
		char ch = (char)c;

		if(ch != '\n'){
			if(index < initialSize)
				word[index++] = ch;
			else{
				initialSize += 20;
				word = (char *)realloc(word, initialSize * sizeof(char));
				word[index++] = ch;
			}
		}
		else
			break;
	}

	return word;
}

void separate_inputs(Request * req, char * totalReq){
	// Logic to separate different components in entire request

	// 1-) GET NUMBER OF FILES
	const int LEN_FILE = strlen(totalReq) + 1;
	//char * numberOfFiles = (char *)calloc(LEN_FILE, sizeof(char));
	char numberOfFiles[LEN_FILE];
	strcpy(numberOfFiles, totalReq);

	//Convert to number
	long int file_count = atoi(numberOfFiles);

	// Update totalReq to the next element
	totalReq += LEN_FILE;

	// 2-) GET PID
	const int LEN_PID = strlen(totalReq) + 1;
	//char * pid = (char *)calloc(LEN_PID, sizeof(char));
	char pid[LEN_PID];
	strcpy(pid, totalReq);

	//Convert to number
	int pid_no = atoi(pid);

	// Update totalReq to the next element
	totalReq += LEN_PID;

	// 3-) GET WORD_CHAR_MODULE
	const int MODULE_LEN = strlen(totalReq) + 1;
	char * module_name = (char *) calloc(MODULE_LEN, sizeof(char));
	strcpy(module_name, totalReq);

	// Update totalReq to the next element
	totalReq += MODULE_LEN;

	// 4-) GET FREQUENCY
	const int FREQ_LEN = strlen(totalReq) + 1;
	//char * freq = (char *) calloc(FREQ_LEN, sizeof(char));
	char freq[FREQ_LEN];
	strcpy(freq, totalReq);

	//Convert to number
	int freq_no = atoi(freq);

	// Update totalReq
	totalReq += FREQ_LEN;

	// 5-) GET STOP_WORDS FILE
	const int STOP_LEN = strlen(totalReq) + 1;
	char * stop_words = (char *) calloc(STOP_LEN, sizeof(char));
	strcpy(stop_words, totalReq);

	//Update totalReq
	totalReq += STOP_LEN;

	// 6-) HANDLE FILES TO BE READ
	char ** files_to_read = (char **)calloc(file_count ,sizeof(char*));
	for(int i = 0; i < file_count; ++i){
		const int LEN = strlen(totalReq) + 1;
		files_to_read[i] = (char *)calloc(LEN, sizeof(char));
		strcpy(files_to_read[i], totalReq);
		totalReq += LEN;
	}

	//Initialize Request struct

	//Number of files to read
	req->number_of_files = file_count;

	//Process id
	req->pid = (pid_t)pid_no;

	//Module name
	req->word_c_module = module_name;

	//Frequency
	req->freq = freq_no;

	//Stop_words
	req->stop_words = stop_words;

	//Unkown number of files to read
	req->files_to_read = files_to_read;

}

void send_results(Tree * norm, Tree * ign, int freq, char * client_fifo){
	Node * most_freq = NULL;
	Node * tmp = NULL;

	int fd = -1;
	if((fd = open(client_fifo, O_WRONLY)) < 0){
		exit(EXIT_FAILURE);
	}

	if(ign->root == NULL || norm->root == NULL)
		return;

	for (int i = 0; i < freq; ++i) {

		do{
			tmp = most_freq;
			norm->most_occuring_word(norm, &most_freq);
			most_freq->visited = true;
			if (tmp == most_freq) break;

		} while(ign->is_present(ign, most_freq->word));

		if(most_freq != tmp){

			//Current Node's properties
			char * wr = most_freq->word;
			int cn = most_freq->count;

			//Stringfy most_freq_count
			char count [BUF_SIZE] = {0};
			sprintf(count, "%d", cn);
			const int LEN = strlen(count) + 1;
			count[LEN] = '\n'; //Marks the end.

			//Create buffer and send the information
			const int WORD_LEN = strlen(wr) + 1;
			char buffer[WORD_LEN + LEN + 1];
			strcpy(buffer, wr);
			buffer[WORD_LEN - 1] = '\0';
			memcpy(buffer + WORD_LEN, count, LEN + 1);

			write(fd, buffer, sizeof(buffer));
		}
	}
	close(fd);
}

void cleanUpMemory(Tree * norm, Tree * ign){
	norm->delete_tree(norm);
	ign->delete_tree(ign);
}

void cleanUpReq(Request * req){
	free(req->word_c_module);
	free(req->stop_words);
	for(int i = 0; i < req->number_of_files; ++i){
		free((req->files_to_read)[i]);
	}
	free(req->files_to_read);
}
