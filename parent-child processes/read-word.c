#include "word-store.h"

//This function reads words from a file and sends each word through a pipe.
void read_and_send_words(char * filename, int fd []){

	//Close Read end of file descriptor. It is not needed.
	close(fd[READ]);

	//Declare file descriptor variable.
	FILE * file;

	//Try to open the file
	if ((file = fopen(filename, "r")) == NULL) {
		printf("File \"%s\" didn't open. Please try to run the program once again with correct arguments\n", filename);
		printf("Process opening \"%s\" is exiting..., PID : %ld\n", filename, (long)getpid());
		close(fd[WRITE]);
		exit(1);
	}

	//Check if the file is empty.
	{
		fseek(file, 0, SEEK_END);
		if(ftell(file) == 0){
			close(fd[WRITE]);
			fclose(file);
			return;
		}
		rewind(file);
	}

	//Variables for both characters and EOF. CH is to be used inside while loop.
	char ch = 0;
	int CH = 0;

	//Other variables needed.
	int flagFirstChar = 1;
	char * word = NULL;
	int wordSize = 20;

	//Index.
	int index = 0;

	//EOF value returned from fgetc can be everything. So it should be stored as an integer
	while ((CH = tolower(fgetc(file))) != EOF) {

	    //Cast CH to char ch to ensure safety of EOF value. It is an integer value not an char type. So can be more than a byte.
		ch = (char)CH;

		//If program hits end of a word.
		if (ch == '\n' || ch == ' ' || ch == '\t' || ((!isalnum(ch)) && (ch != '\''))) {

			//If there is a word execute following if statement
			if (word != NULL) {
				//If word array for the string doesn't have a place for Null terminator allocate space with one more
				if (index == wordSize) {
					wordSize += 1;
					if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
						fprintf(stderr, "A problem occurred during realloc, please try running the program once again\n");
						printf("Process opening \"%s\" is exiting..., PID : %ld\n", filename, (long)getpid());
						close(fd[WRITE]);
						exit(1);
					}
				}
				word[index++] = '\0';
				write(fd[WRITE], word, strlen(word) + 1);
				free(word);
			}

			//Set necessary variables for flow of the program
			flagFirstChar = 1;
			index = 0;
			wordSize = 20;
			word = NULL;
		}

		else if (isalnum(ch) || ch == '\'') {
			//Marks beginning of a word.
			if (flagFirstChar == 1) {
				//In the beginning of the word, take 20 byte place for a word.
				if ((word = (char*)malloc(sizeof(char) * wordSize)) == NULL) {
					fprintf(stderr, "A problem occurred during malloc, please try running the program once again\n");
					printf("Process opening \"%s\" is exiting..., PID : %ld\n", filename, (long)getpid());
					close(fd[WRITE]);
					exit(1);
				}
				memset(word, 0, wordSize * sizeof(char));
				flagFirstChar = 0;
			}
			//If already allocated space is NOT sufficient, allocate more space.
			if (index == wordSize) {
				//tmpSize is to be used for initializing non-used parts of char array to zero.
				int tmpSize = wordSize;
				wordSize += 20;
				if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
					fprintf(stderr, "A problem occurred during realloc, please try running the program once again\n");
					printf("Process opening \"%s\" is exiting..., PID : %ld\n", filename, (long)getpid());
					close(fd[WRITE]);
					exit(1);
				}
				//Set rest of the new space to 0
				memset(word + tmpSize, 0, 20 * sizeof(char));
			}
			word[index++] = ch;
		}
	}

	if (CH == EOF && word != NULL) {
		//If word array for the string doesn't have a place for Null terminator.
		if (index == wordSize) {
			wordSize += 1;
			if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
				fprintf(stderr, "A problem occurred during realloc, please try running the program once again\n");
				printf("Process opening \"%s\" is exiting..., PID : %ld\n", filename, (long)getpid());
				close(fd[WRITE]);
				exit(1);
			}
		}
		word[index++] = '\0';
		write(fd[WRITE], word, strlen(word) + 1);
		free(word);
		word = NULL;
	}

	//Close the file when all operations are done
	fclose(file);

	//Close write end because it is no longer needed
	close(fd[WRITE]);
}
