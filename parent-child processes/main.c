#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "word-store.h"

typedef struct pair{
	char * word;
	int n;
} Pair;

void read_pipe(int fd, Pair *);
void masterProcessWordRead(int [], Tree *);
void masterProcessWordDisplay(Tree *, Tree *, int);
void cleanUpMemory(Tree *, Tree *);

int main(int argc, char * argv []){

    char * freq = argv[1];      //Most frequent words,
    char * ignWrd = argv[2];    //Words to be ignored.

	//Input Tests.
	if (argc < 4) {
		fprintf(stderr, "At least 3 arguments must be provided:\n");
		fprintf(stderr, "1-)How many words to be printed(numerical input)\n");
		fprintf(stderr, "2-)A file that contains words that are not to be displayed\n");
		fprintf(stderr, "3-)As many files as you wish that contains words to be printed\n");
		fprintf(stderr, "Exiting... Please try again\n");
		exit(1);
	}

	for (unsigned int i = 0; i < strlen(freq); ++i) {
		if (freq[i] == '-') {
			fprintf(stderr, "First command line input must be a non-negative number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}

		else if (!isdigit(freq[i])) {
			fprintf(stderr, "First command line input must be a number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}
	}

	//Hold words to be ignored
	Tree igWords;

	//Hold words to be printed
	Tree normalWords;

	//Call necessary functions for program flow
	{
		binary_tree_ctor(&igWords);
		binary_tree_ctor(&normalWords);
	}

	read_words(ignWrd, &igWords);

	//File descriptor arrays for pipes
	//DESC_SIZE stands for file descriptor size and it is defined in word-store.h header file
	const int fixedArgs = 3;
	const int numReads = argc - fixedArgs;

    int fds[numReads][F_DESCRIPTOR_SIZE];

	for(int i = 0; i < numReads; ++i){
		if(pipe(fds[i]) < 0){
			perror("Pipe Error");
			exit(1);
		}
	}

	//Declare process variable
	pid_t pid, wait_pid;

	int status = 0;

	//Read words to be printed
	for (int i = 0; i < numReads; ++i) {
		if((pid = fork()) < 0){
			perror("Fork Error");
			exit(1);
		}
		else if(pid > 0){

			masterProcessWordRead(fds[i], &normalWords);

		}
		else if(pid == 0){
			//childProcess
			read_and_send_words(argv[i + fixedArgs], fds[i]);
			exit(0);
		}
	}

	//Function to display true frequencies.
	masterProcessWordDisplay(&normalWords, &igWords, atoi(freq));

	//Memory CleanUp
	cleanUpMemory(&normalWords, &igWords);

	//Wait for all child processes to terminate.
	while((wait_pid = wait(&status)) > 0);

	return 0;
}

void masterProcessWordRead(int fd [], Tree * tr){
	close(fd[WRITE]);

	Pair p;

	do{
		read_pipe(fd[READ], &p);

		if(p.n)
		{
			tr->insert_word(tr, p.word);
		}

	}while(p.n);

	close(fd[READ]);
}

void masterProcessWordDisplay(Tree * norm, Tree * ign, int freq) {

	Node * most_freq = NULL;
	Node * tmp = NULL;

	if(ign->root == NULL || norm->root == NULL)
		return;

	for (int i = 0; i < freq; ++i) {

		do{
			tmp = most_freq;
			norm->most_occuring_word(norm, &most_freq);
			most_freq->visited = true;
			if (tmp == most_freq) break;

		} while(ign->is_present(ign, most_freq->word));

		if(most_freq != tmp)
			printf("%s %d\n", most_freq->word, most_freq->count);
	}

}

void cleanUpMemory(Tree * norm, Tree * ign){
	norm->delete_tree(norm);
	ign->delete_tree(ign);
}

void read_pipe(int fd, Pair * p){

	char * word = NULL;
	int n = -1;
	char var = -1;
	int initialSize = 20;
	int index = 0;

	p->n = 1;

	if((word = (char *)malloc(initialSize * sizeof(char))) == NULL){
		fprintf(stderr, "A problem occured during calloc in the main process during data read from child processes");
		// DO NOT FORGET TO TERMINATE CHILD PROCESSES.
		exit(1);
	}

	do{
		if((n = read(fd, &var, 1)) == 0){
			p->n = 0;
			free(word);
			word = NULL;
			p->word = NULL;
			return;
		}

		else{
			if(var != '\0'){
				if(index < initialSize){
					word[index++] = var;
				}else{
					initialSize += 20;
					word = (char *)realloc(word, initialSize * sizeof(char));
				}
			}
			else{
				if(index < initialSize){
					word[index++] = '\0';
				}else{
					initialSize += 1;
					word = (char *)realloc(word, initialSize * sizeof(char));
					word[index++] = '\0';
				}
				break;
			}
		}

	} while(n > 0);

	p->word = word;

}

