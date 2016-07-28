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

#include "fifo_def_client.h"

void input_tests(char **, int);
void generate_request(Request *, char **);
char * concat(char *, char *);

void generateServerFifoPath(char *, char *);
void generateClientFifoPath(char *, char *, int);
void send_request(Request *, int, int);

void display_res(char *);

void have_response(int);

int main(int argc, char * argv []) {

	//First test inputs

	input_tests(argv, argc);

	//All needed declarations

	int request_fifo;
	int response_fifo;

	char * dir_name = argv[1];
	const int CLIENT_FIFO_NAME_LEN = strlen(dir_name) + strlen(CLIENT_FIFO_TEMPLATE) + PADDING;
	const int SERVER_FIFO_NAME_LEN = strlen(dir_name) + strlen(SERVER_FIFO) + 1;

	char serverFifo[SERVER_FIFO_NAME_LEN];
	generateServerFifoPath(dir_name, serverFifo);

	char clientFifo[CLIENT_FIFO_NAME_LEN];
	generateClientFifoPath(dir_name, clientFifo, CLIENT_FIFO_NAME_LEN);

	//Create client-specific FIFO
	umask(0);
	if(mkfifo(clientFifo, 0777) == -1){
		perror("FIFO creation error");
	}

	//Construct request message, open request FIFO and send request
	Request req;
	generate_request(&req, argv);
	if ((request_fifo = open(serverFifo, O_WRONLY)) < 0){
		perror("Server FIFO open error");
	}

	send_request(&req, request_fifo, argc);

	//Wait for response to come.
	if((response_fifo = open(clientFifo, O_RDONLY)) < 0){
		perror("Client FIFO open error");
	}

	//Also displays the result
	have_response(response_fifo);

	//Close file descriptors
	close(request_fifo);
	close(response_fifo);

	//Delete client specific FIFO.
	unlink(clientFifo);

	return 0;
}

void input_tests(char ** argv, int argc){

	char * dir_name = argv[1];      //Most frequent words,
	char * freq = argv[3];

	//Input if enough arguments are given.
	if (argc < 5) {
		fprintf(stderr, "At least 5 command-line arguments must be provided to this program:\n");
		fprintf(stderr, "1-)Working directory path of the server process\n");
		fprintf(stderr, "2-)Path of char_module to be used\n");
		fprintf(stderr, "3-)Number of most frequent words to be printed\n");
		fprintf(stderr, "4-)A file containing the words not to be printed\n");
		fprintf(stderr, "5-)At least one file containing the words to be printed\n");
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

void generate_request(Request * st, char ** argv){
	st->word_c_module = argv[2];
	st->most_freq_word = argv[3];
	st->stop_words = argv[4];
	st->files_to_read = argv + 5;
	st->pid = getpid();
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

void generateClientFifoPath(char * dir_name, char * clientFifo, int size){
	char * client_place = concat(dir_name, CLIENT_FIFO_TEMPLATE);
	snprintf(clientFifo, size, client_place, (long) getpid());
	free(client_place);
}

void send_request(Request * req, int fd, int arg_count){

	const int neccessary_arg = 5;
	const int file_number = arg_count - neccessary_arg;

	//Buffer to convert a integer into a string
	char buffer_pid[BUF_SIZE] = {0};
	char buffer_int[BUF_SIZE] = {0};

	const char ENDING = '\0';
	const char REQ_END = '\n';

	sprintf(buffer_pid, "%d", req->pid);
	sprintf(buffer_int, "%d", file_number);

	const int FILE_NUMB_SIZE = strlen(buffer_int) + 1;
	const int PID_SIZE = strlen(buffer_pid) + 1;

	buffer_int[FILE_NUMB_SIZE - 1] = ENDING;
	buffer_pid[PID_SIZE - 1] = ENDING;

	const int WORD_MOD_SIZE = strlen(req->word_c_module) + 1;
	const int STOP_WORDS_SIZE = strlen(req->stop_words) + 1;
	const int MOST_FREQ_SIZE = strlen(req->most_freq_word) + 1;

	const int TOTAL_INITIAL = FILE_NUMB_SIZE + PID_SIZE + WORD_MOD_SIZE + MOST_FREQ_SIZE + STOP_WORDS_SIZE;

	char word_module[WORD_MOD_SIZE];
	strcpy(word_module, req->word_c_module);
	word_module[WORD_MOD_SIZE - 1] = ENDING;

	char most_freq[MOST_FREQ_SIZE];
	strcpy(most_freq, req->most_freq_word);
	most_freq[MOST_FREQ_SIZE - 1] = ENDING;

	char stop_words[STOP_WORDS_SIZE];
	strcpy(stop_words, req->stop_words);
	stop_words[STOP_WORDS_SIZE - 1] = ENDING;

	int TEMP_TOTAL = TOTAL_INITIAL;
	for(int i = 0; i < file_number; ++i){
		TEMP_TOTAL += strlen((req->files_to_read)[i]) + 1;
	}

	char entireRequest[TEMP_TOTAL + 1]; //+1 is for \n delimiter indicating end of a request
	//How many files to be read
	memcpy(entireRequest, buffer_int, FILE_NUMB_SIZE);
	//Pid number
	memcpy(entireRequest + FILE_NUMB_SIZE, buffer_pid, PID_SIZE);
	//word_module
	memcpy(entireRequest + FILE_NUMB_SIZE + PID_SIZE, word_module, WORD_MOD_SIZE);
	//freq
	memcpy(entireRequest + FILE_NUMB_SIZE + PID_SIZE + WORD_MOD_SIZE , most_freq, MOST_FREQ_SIZE);
	//most freq
	memcpy(entireRequest + FILE_NUMB_SIZE + PID_SIZE + WORD_MOD_SIZE + MOST_FREQ_SIZE , stop_words, STOP_WORDS_SIZE);

	int tmpSize = TOTAL_INITIAL;
	for(int i = 0; i < file_number; ++i){
		char * file = (req->files_to_read)[i];
		int len = strlen(file);
		char tmp [len + 1];
		strcpy(tmp, file);
		tmp[len] = ENDING;

		memcpy(entireRequest + tmpSize, tmp, sizeof(tmp));

		tmpSize += len + 1;
	}

	//Newline character indicates end of a request
	entireRequest[TEMP_TOTAL] = REQ_END;
	write(fd, entireRequest, sizeof(entireRequest));
}

void have_response(int fd){

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

		else{
			// If \n comes then print it
			display_res(word);
			free(word);

			//Reset because incoming values will be new ones
			initialSize = 20;
			index = 0;
			word = (char *)calloc(initialSize, sizeof(char));

			continue;
		}
	}

}

void display_res(char * response){

	//Use word buf
	const int WORD_LEN = strlen(response) + 1;
	char word_buf[WORD_LEN];
	strcpy(word_buf, response);

	//Update word
	response += WORD_LEN;

	//Find count and convert
	const int COUNT_LEN = strlen(response) + 1;
	char count_buf[COUNT_LEN];
	strcpy(count_buf, response);

	int count = atoi(count_buf);

	printf("%s %d\n", word_buf, count);
}
