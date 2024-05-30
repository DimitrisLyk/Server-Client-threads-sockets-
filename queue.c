#include <stdlib.h>
#include "queue.h"

//Αρχικοποιω την ουρα θετοντας head,tail με NULL, size = queue size που πηραμε σαν argument
void Queue_Create(queue* q, int size) {
    q->head = NULL;
    q->tail = NULL;
    q->size = size;  //
    q->connections = 0;               //Ποσες συνδεσεις εχει αυτη τη στιγμη μεσα το queue
}

//Προσθετω ενα στοιχειο (μια συνδεση) μεσα στο queue
void enqueue(queue* q, int* client_socket) {
    node* newnode = malloc(sizeof(node));       //Δεσμευω χωρο για το στοιχειο που θα προσθεσω
    newnode->client_socket = client_socket;     //Εκχωρω τον socket descriptor στο node
    newnode->next = NULL;                       //Θετω το επομενο σε NULL
    if (q->tail != NULL) {                      //Εαν η ουρα δεν ειναι NULL, δλδ το queue δεν ειναι κενο, ενημερωνω τον επομενο δεικτη
        q->tail->next = newnode;                //Της τρεχουσας ουρας στον νεο κομβο
    }
    q->tail = newnode;                          //Οριζω τον δεικτη της ουρας στο νεο κομβο
    if (q->head == NULL) {                      //Αν το head ειναι NULL δλδ το queue ειναι κενο
        q->head = newnode;                      //Ενημερωνω τον δεικτη head στον νεο κομβο
    }
    q->connections++;                           //Αυξανω τις current συνδεσεις κατα 1
}

//Αφαιρω ενα στοιχειο απτο queue
int* dequeue(queue* q) {
    if (q->head == NULL) {                      //Αν το head ειναι NULL δλδ το queue κενο επιστρεφει NULL
        return NULL;                            //Στη δικια μας περιπτωση δεν μπαινει ο κωδικας ποτε μεσα σε αυτο το if
    } else {                                    //Επειδη ελεγχουμε εαν ειναι κενο πριν αφαιρεσουμε με τη queue is empty
        int* result = q->head->client_socket;   //Διαφορετικα ανακτω τον socket descriptor απο τον κομβο head
        node* temp = q->head;                   
        q->head = q->head->next;                //Ενημερωνω τον δεικτη head στον επομενο κομβο
        if (q->head == NULL) {
            q->tail = NULL;
        }
        free(temp);                             //Ελευθερωνω τη μνημη του αρχικου κομβο head αφου το αφαιρεσα
        q->connections--;                       //Μειωνω τις συνδεσεις κατα 1
        return result;                          //Επιστρεφω την τιμη του socket descr
    }
}

int Queue_IsEmpty(queue* q) {                   //Ελεγχω αν το queue ειναι αδεια
    //return (q->head == NULL);
    return (Queue_GetSize(q) == 0);                   //Επιστρεφω 1 αν ειναι και 0 αν δεν ειναι
}


int Queue_IsFull(queue* q) {                    //Ελεγχω εαν το queue ειναι γεματο
    return (Queue_GetSize(q) == q->size);       //Ελεγχω εαν οι current συνδεσεις με τη βοηθεια του get size ισουται με το max size του queue
}                                               //Που εχουμε ορισει

int Queue_GetSize(queue* q) {                   //Επιστρεφει τον αριθμο των συνδεσεων που υπαρχουν αυτη τη στιγμη στο queue
   return q->connections;
}
