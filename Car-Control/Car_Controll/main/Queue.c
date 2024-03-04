#include "Queue.h"

void queue_init(Queue* q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_discard_half(Queue* q) {
    int itemsToDiscard = q->size / 2;
    for (int i = 0; i < itemsToDiscard; ++i) {
        void* data = queue_dequeue(q);
        // If the data was dynamically allocated, ensure you free it here
        free(data);
    }
}

void queue_enqueue(Queue* q, void* data) {
    // Check if queue size has reached its maximum limit
    if (q->size >= 20) {
        // Discard half of the items
        queue_discard_half(q);
    }

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
/* When dequeueing, the returned data needs a cast to (*expected_data_type)*/
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

void enqueue_words(Queue* q, const char* str) {
    if (str == NULL) return; // Safety check

    char* word = (char*)malloc(strlen(str) + 1); // Allocate memory for the string
    if (word == NULL) {
        perror("Failed to allocate memory for word");
        return;
    }
    strcpy(word, str); // Copy the string into the newly allocated memory
    queue_enqueue(q, word); // Enqueue the copied string
}



