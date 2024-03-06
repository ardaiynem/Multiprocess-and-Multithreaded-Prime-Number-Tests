#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_
#include <stdio.h>
#include <stdlib.h>

struct Node
{
    int item;
    struct Node *next;
};

void insert(struct Node **head, int item);

void freeList(struct Node *head);

#endif