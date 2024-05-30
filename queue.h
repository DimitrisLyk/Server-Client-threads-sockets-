#ifndef QUEUE_H
#define QUEUE_H


//struct node που αναπαριστα ενα κομβο στην ουρα
//Καθε node περιλαμβανει εναν pointer που δειχνει στον επομενο κομβο
//Και ενα δεικτη για την καθε συνδεση που δεχομαστε
typedef struct node {
    struct node* next;
    int* client_socket;
} node;

//struct queue που αναπαριστα το ιδιο το queue
//Εχει μια κεφαλη, μια ουρα, μεγεθος, και το ποσα connections εχει μεσα
typedef struct {
    node* head;
    node* tail;
    int size;
    int connections;
} queue;

//Δηλωση συναρτησεων που χρησιμοποιουμε
void Queue_Create(queue* q, int size);
void enqueue(queue* q, int* client_socket);
int* dequeue(queue* q);
int Queue_IsEmpty(queue* q);
int Queue_GetSize(queue* q);
int Queue_IsFull(queue* q);


#endif

