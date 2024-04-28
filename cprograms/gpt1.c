#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAP_SIZE 1024 * PAGE_SIZE // about 1 Gigabyte
#define PAGE_SIZE 4096      // 4KB

char *heap;

void init_heap() {
    heap = (char *)malloc(HEAP_SIZE);
    if (!heap) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    // We only want to load on first access
    // memset(heap, 0, HEAP_SIZE);
}

void clear_heap() {
    memset(heap, 0, HEAP_SIZE);
}

void draw(int n) {
    if (n < 1 || n > HEAP_SIZE / PAGE_SIZE) {
        printf("Invalid number of segments.\n");
        return;
    }

    int pages_per_segment = (HEAP_SIZE / PAGE_SIZE) / n;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < pages_per_segment; j++) {
            int pattern_type = i % 3; // Determine pattern based on segment index
            int page_index = i * pages_per_segment + j;

            if (pattern_type == 0 || (pattern_type == 1 && j % 2 == 0) || (pattern_type == 2 && j % 2 != 0)) {
                if (page_index * PAGE_SIZE < HEAP_SIZE) {
                    heap[page_index * PAGE_SIZE] = 1; // Set first byte of the page
                }
            }
        }
    }
}

int main() {
    init_heap(); // Initialize and zero out the heap

    char command[10];
    int segments;

    printf("Enter command (Draw <N> or Clear):\n");
    while (scanf("%s", command) != EOF) {
        if (strcmp(command, "Draw") == 0) {
            if (scanf("%d", &segments) == 1) {
                draw(segments);
            } else {
                printf("Invalid command format.\n");
            }
        } else if (strcmp(command, "Clear") == 0) {
            clear_heap();
        } else {
            printf("Unknown command.\n");
        }
        printf("Enter command (Draw <N> or Clear):\n");
    }

    free(heap);
    return 0;
}
