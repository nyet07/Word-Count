#include "word-store.h"

void binary_tree_ctor(Tree * tree){
    tree->insert_word = &insert_word;
    tree->delete_tree = &delete_tree;
    tree->most_occuring_word = &most_occuring_word;
	tree->list_elements = &list_elements;
	tree->is_present = &is_present;

	tree->root = NULL;
}

void insert_word(Tree * tree, char * arr) {
	insert_word_aux(&(tree->root), arr);
}

void insert_word_aux(Node ** p, char * arr){
	
	if(*p == NULL){
		if ((*p = (Node *)malloc(sizeof(Node))) == NULL) {
			fprintf(stderr, "A problem occured during malloc, please try running the program once again\n");
			exit(1);
		}
		(*p)->count = 1;
		(*p)->word = arr;
		(*p)->visited = false;
		(*p)->left_node = NULL;
		(*p)->right_node = NULL;
    }

	else if (strcmp(arr, (*p)->word) > 0) {
//		insert_word_aux(&(*p)->right_node, arr);
		insert_word_aux(&(*p)->left_node, arr);
	}
	else if (strcmp(arr, (*p)->word) < 0) {
//		insert_word_aux(&(*p)->left_node, arr);
		insert_word_aux(&(*p)->right_node, arr);

	}
	else {
		(*p)->count++;
		free((*p)->word);
		(*p)->word = arr;
	}
}

bool is_present(Tree * tp, char * word) {
	return is_present_aux(tp->root, word);
}

bool is_present_aux(Node * p, char * word) {
	if (p == NULL) 
		return false;
	
	else if (strcmp(word, p->word) > 0) {
//		return is_present_aux(p->right_node, word);
		return is_present_aux(p->left_node, word);
	}

	else if (strcmp(word, p->word) < 0){
//		return is_present_aux(p->left_node, word);
		return is_present_aux(p->right_node, word);
	}
	
	else
		return true;
}

void delete_tree(Tree * p){
	postorder_traversal_destructor(p->root);
}

void postorder_traversal_destructor(Node * p){
	if (p == NULL) return;
	
	postorder_traversal_destructor(p->left_node);
	postorder_traversal_destructor(p->right_node);
	
	free(p->word);
	free(p);
}

void most_occuring_word(Tree * p, Node ** result) {
	bool flag = false;
	preorder_traversal_aux(p->root, result, &flag);
}

void preorder_traversal_aux(Node * p, Node ** result, bool * flag) {
	if (p != NULL) {
		if (!p->visited && !(*flag)) { //if p unvisited
			*result = p;
			*flag = true;
		}
		else {
			if (p->count > (*result)->count && p->visited != true) {
				*result = p;
			}
		}

		preorder_traversal_aux(p->left_node, result, flag);
		preorder_traversal_aux(p->right_node, result, flag);
	}
}

void list_elements(Tree * p) {
	inorder_traversal_aux(p->root);
}

void inorder_traversal_aux(Node * p) {
	if (p != NULL) {
		inorder_traversal_aux(p->left_node);
		printf("string: %s and count: %d and visited %d\n", p->word, p->count, p->visited);
		inorder_traversal_aux(p->right_node);
	}
}

void read_words(char * filename, Tree * tp) {
	
	FILE * file;

	if ((file = fopen(filename, "r")) == NULL) {
		printf("File \"%s\" didn't open. Please try to run the program once again with correct arguments\n", filename);
		exit(1);
	}
	
	//Check if the file is empty.
	{
		fseek(file, 0, SEEK_END);
		if(ftell(file) == 0){
			fclose(file);
			return;
		}
		rewind(file);
	}

	char ch = 0;
	int CH = 0;

	int flagFirstChar = 1;
	char * word = NULL;
	int wordSize = 20;

	int index = 0;

	while ((CH = tolower(fgetc(file))) != EOF) { //Case doesn't matter, all is identical
		
		ch = (char)CH; //Cast CH to char ch to ensure safety of EOF value. It is an integer value not an char type. So can be more than a byte.

		if (ch == '\n' || ch == ' ' || ch == '\t' || ((!isalnum(ch)) && (ch != '\''))) {
			
			if (word != NULL) { //If program hits end of a word.
				if (index == wordSize) { //If word array for the string doesn't have a place for Null terminator.
					wordSize += 1;
					if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
						fprintf(stderr, "A problem occured during realloc in master the process, please try running the program once again\n");
                        exit(1);
					}
				}
				word[index++] = '\0';
				tp->insert_word(tp, word);
			}

			flagFirstChar = 1;
			index = 0;
			wordSize = 20;
			word = NULL;
		}

		else if (isalnum(ch) || ch == '\'') {
			if (flagFirstChar == 1) { //Marks beginning of a word.
				if ((word = (char*)malloc(sizeof(char) * wordSize)) == NULL) {//In the beginning of the word, take 20 byte place for a word.
					fprintf(stderr, "A problem occured during malloc in the master process, please try running the program once again\n");
					exit(1);
				}
				memset(word, 0, wordSize);
				flagFirstChar = 0;
			}
			if (index == wordSize) {
				wordSize += 20;
				if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
					fprintf(stderr, "A problem occured during realloc in the master process, please try running the program once again\n");
					exit(1);
				}
			}
			word[index++] = ch;
		}
	}

	if (CH == EOF && word != NULL) {
		if (index == wordSize) { //If word array for the string doesn't have a place for Null terminator.
			wordSize += 1;
			if ((word = (char *)realloc(word, sizeof(char) * wordSize)) == NULL) {
				fprintf(stderr, "A problem occured during realloc in the master process, please try running the program once again\n");
                exit(1);
			}
		}
		word[index++] = '\0';
		tp->insert_word(tp, word);
		word = NULL;
	}

	fclose(file);
}
