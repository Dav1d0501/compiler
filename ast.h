#ifndef AST_H
#define AST_H
#include "symtab.h"

typedef struct node {
    char *token;
    DataType eval_type; // סוג הטיפוס שנבדק
    struct node *left;
    struct node *right;
} node;

node *mknode(char *token, node *left, node *right);
void printtree(node *tree,int depth);

#endif