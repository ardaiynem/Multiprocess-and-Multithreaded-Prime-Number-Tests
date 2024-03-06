#include "linkedList.h"

void insert(struct Node **head, int item)
{
    // Case 1: Empty list
    if (*head == NULL)
    {
        *head = (struct Node *)malloc(sizeof(struct Node));
        (*head)->item = item;
        (*head)->next = NULL;
        return;
    }

    // Case 2: Non-empty list
    struct Node *ptrNode = *head;
    while (ptrNode->next != NULL)
    {
        ptrNode = ptrNode->next;
    }

    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->item = item;
    newNode->next = NULL;
    ptrNode->next = newNode;
}

void freeList(struct Node *head)
{
    struct Node *current = head;
    while (current != NULL)
    {
        struct Node *next = current->next;
        free(current);
        current = next;
    }
}