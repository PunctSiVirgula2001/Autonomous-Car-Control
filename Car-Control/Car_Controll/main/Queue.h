#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Node structure
typedef struct Node {
    void* data;
    struct Node* next;
} Node;

// Queue structure
typedef struct {
    Node* head;
    Node* tail;
    int size;
} Queue;

// Function prototypes
void queue_init(Queue* q);
void queue_enqueue(Queue* q, void* data);
void queue_discard_half(Queue* q);
void* queue_dequeue(Queue* q);
int queue_size(Queue* q);
int queue_is_empty(Queue* q);
void queue_clear(Queue* q, void (*free_data)(void*));

// Helper function to enqueue a word
void enqueue_word(Queue* q, const char* start, size_t length);
// Function to enqueue every word that ends with a newline
void enqueue_words_with_newline(Queue* q, const char* str);

void enqueue_words(Queue* q, const char* str);

