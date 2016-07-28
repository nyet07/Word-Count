#include <stdio.h>
#include <ctype.h>
#include "word-store.h"

int main(int argc, char * argv []){

	Tree igWords;
	Tree normalWords;

	binary_tree_ctor(&igWords);
	binary_tree_ctor(&normalWords);

    //N, stop-words, File1 .....

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
			fprintf(stderr, "First commandline input must be a non-negative number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}

		else if (!isdigit(freq[i])) {
			fprintf(stderr, "First commandline input must be a number!\n");
			fprintf(stderr, "Exiting... Please try again\n");
			exit(1);
		}
	}

	//Read words to be ignored
	read_words(ignWrd, &igWords);

	//Read words to be printed
	for (int i = 3; i < argc; ++i) {
		read_words(argv[i], &normalWords);
	}
	
	//most_freq variable will have string with most repetitions.
	Node * most_freq = NULL;
	Node * tmp = NULL;

	for (int i = 0; i < atoi(freq); ++i) {
		normalWords.most_occuring_word(&normalWords, &most_freq);
		while(igWords.is_present(&igWords, most_freq->word)) {
			most_freq->visited = true;
			tmp = most_freq;
			normalWords.most_occuring_word(&normalWords, &most_freq);
			if (tmp == most_freq) break;
		}
		most_freq->visited = true;
		if (most_freq != tmp) 
			printf("%s %d\n", most_freq->word, most_freq->count);
		tmp = most_freq;
	}

	igWords.delete_tree(&igWords);
	normalWords.delete_tree(&normalWords);

	return 0;
}
