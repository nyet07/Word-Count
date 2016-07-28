#ifndef WORD_STORE_H
#define WORD_STORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>

enum{				//Enumerated types are new keywords not strings.
	READ,
	WRITE,
	F_DESCRIPTOR_SIZE,
};

typedef struct Node{
    struct Node * right_node;
    struct Node * left_node;

	//Data state.
    unsigned int count;
    char * word;
	bool visited;

} Node;

typedef struct BinaryTree{

    Node * root;

    //Functions are declared here.
    void (*insert_word)(struct BinaryTree *, char *);
    void (*delete_tree)(struct BinaryTree *);
	void (*list_elements)(struct BinaryTree *);
	void (*most_occuring_word)(struct BinaryTree *, Node ** result);
	bool (*is_present)(struct BinaryTree *, char *);

} Tree;

//Tree Constructor Function
void binary_tree_ctor(Tree *);

//Insert if doesn't exist. If exists increment count.
void insert_word(Tree *, char *);

//Helper function for inserting word.
void insert_word_aux(Node **, char *);

//Check if a word is present
bool is_present(Tree *, char *);

//is_present helper function
bool is_present_aux(Node *, char *);

//Delete entire tree.
void delete_tree(Tree * p);

//Traverse Tree For Destruction
void postorder_traversal_destructor(Node *);

//Node pointer's address is passede to second parameter.
void most_occuring_word(Tree * , Node ** );

//Traverse Tree Aux
void preorder_traversal_aux(Node *, Node **, bool *);

//List elements a tree has.
void list_elements(Tree * p);

//Helper Function
void inorder_traversal_aux(Node *);

//Read words from ignorance
void read_words(char *, Tree *); 

//Read words from a file and sends them through a pipe.
void read_and_send_words(char *, int []);

#endif
