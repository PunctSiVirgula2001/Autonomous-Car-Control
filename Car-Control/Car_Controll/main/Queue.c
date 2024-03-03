#include "Queue.h"

void queue_init(Queue* q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_enqueue(Queue* q, void* data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Failed to allocate new node");
        return;
    }
    newNode->data = data;
    newNode->next = NULL;

    if (q->tail == NULL) {
        q->head = newNode;
        q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail = newNode;
    }
    q->size++;
}

void* queue_dequeue(Queue* q) {
    if (q->head == NULL) return NULL;

    Node* temp = q->head;
    void* data = temp->data;
    q->head = q->head->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }

    free(temp);
    q->size--;
    return data;
}

int queue_size(Queue* q) {
    return q->size;
}

int queue_is_empty(Queue* q) {
    return q->size == 0;
}

void queue_clear(Queue* q, void (*free_data)(void*)) {
    while (!queue_is_empty(q)) {
        void* data = queue_dequeue(q);
        if (free_data != NULL) {
            free_data(data);
        }
    }
}

// Helper function to enqueue a word
void enqueue_word(Queue* q, const char* start, size_t length) {
    char* word = (char*)malloc(length + 1); // +1 for the null terminator
    if (!word) {
        perror("Failed to allocate memory for word");
        return;
    }
    strncpy(word, start, length);
    word[length] = '\0'; // Null-terminate the string
    queue_enqueue(q, word);
}

// Function to enqueue every word that ends with a newline
void enqueue_words_with_newline(Queue* q, const char* str) {
    const char* word_start = str;
    const char* current = str;

    while (*current) {
        // If a newline character is found, enqueue the word
        if (*current == '\n') {
            enqueue_word(q, word_start, current - word_start + 1); // +1 to include the newline character
            word_start = current + 1; // Move to the next character after the newline
        }
        current++;
    }
}
