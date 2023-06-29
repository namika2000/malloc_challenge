//
// >>>> malloc challenge! <<<<
//
// Your task is to improve utilization and speed of the following malloc
// implementation.
// Initial implementation is the same as the one implemented in simple_malloc.c.
// For the detailed explanation, please refer to simple_malloc.c.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FREE_LIST_BIN_SIZE 11

//
// Interfaces to get memory pages from OS
//
void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

//
// Struct definitions
//
typedef struct my_metadata_t {
  size_t size;
  struct my_metadata_t *next;
} my_metadata_t;

typedef struct my_heap_t {
  my_metadata_t *free_head;
  my_metadata_t dummy;
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
my_heap_t my_heap[FREE_LIST_BIN_SIZE]; 

//
// Helper functions (feel free to add/remove/edit!)
//
// Add a free slot to the beginning of the free list.
void my_add_to_free_list(my_metadata_t *metadata) {
  // printf("%zuの新しいfreeを追加\n", metadata -> size);
  assert(!metadata->next);

  // sizeからどのfree list binに属するか判断
  int i, j, size, max_size;
  size = metadata -> size;
  for (i=1; i <= FREE_LIST_BIN_SIZE - 1; i++) {
    for (j=1; j <= i + 1; j++) {
      max_size *= 2;
    }
    if (!(size <= max_size)) {
      break;
    }
  }
  metadata->next = my_heap[i-1].free_head;
  my_heap[i-1].free_head = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev, int bin_group) {
  // printf("%zuをfreeから削除\n", metadata -> size);

  if (prev) {
    prev->next = metadata->next;
  } else {
    my_heap[bin_group].free_head = metadata->next;
  }
  metadata->next = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

// This is called at the beginning of each challenge.
void my_initialize() {
  // printf("初期化--------------\n");
  int i;
  for (i=0; i <= FREE_LIST_BIN_SIZE - 1; i++) {
    my_heap[i].free_head = &my_heap[i].dummy;
    my_heap[i].dummy.size = 0;
    my_heap[i].dummy.next = NULL;
  }
}

// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().
void *my_malloc(size_t size) {
  // printf("%zuのメモリーが欲しい!!!", size);
  // sizeからどのbinに属するか判断
  int i, j, max_size, bin_group;
  for (i=1; i <= FREE_LIST_BIN_SIZE - 1; i++) {
    for (j=1; j <= i + 1; j++) {
      max_size *= 2;
    }
    if (!(size <= max_size)) {
      break;
    }
  }
  bin_group = i - 1;
  my_metadata_t *metadata = my_heap[bin_group].free_head;
  my_metadata_t *prev = NULL;
  // First-fit: Find the first free slot the object fits.
  // TODO: Update this logic to Best-fit!

  my_metadata_t *current_metadata = my_heap[bin_group].free_head;
  my_metadata_t *prev_metadata = NULL;
  my_metadata_t *best_metadata = NULL;
  my_metadata_t *best_prev_metadata =NULL;
  size_t best_diff = 5000; //ここを追加するメモリ(4080)以下にしていると無限ループ
  while (current_metadata) {
    if (current_metadata -> size >= size) {
      size_t diff = current_metadata -> size - size;
      if (diff < best_diff) {
        best_prev_metadata = prev_metadata;
        best_metadata = current_metadata;
        best_diff = diff;
        if (best_diff == 0) {
          break;
        }
      }
    }

    prev_metadata = current_metadata;
    current_metadata = current_metadata->next;
  }

  // printf("%zu\n", best_diff);
  metadata = best_metadata;
  prev = best_prev_metadata;
  // now, metadata points to the first free slot
  // and prev is the previous entry.

  if (!metadata) {
    // There was no free slot available. We need to request a new memory region
    // from the system by calling mmap_from_system().
    //
    //     | metadata | free slot |
    //     ^
    //     metadata
    //     <---------------------->
    //            buffer_size
    size_t buffer_size = 4096;
    my_metadata_t *metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(my_metadata_t);
    metadata->next = NULL;
    // Add the memory region to the free list.
    // printf("足りなかったのでメモリを追加--------\n");
    my_add_to_free_list(metadata);
    // Now, try my_malloc() again. This should succeed.
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  // Remove the free slot from the free list.
  my_remove_from_free_list(metadata, prev, bin_group);

  if (remaining_size > sizeof(my_metadata_t)) {
    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    metadata->size = size;
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    // printf("残りの領域をfreeに追加-----\n");
    my_add_to_free_list(new_metadata);
  }
  return ptr;
}

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  // printf("freeする---------\n");
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  // Add the free slot to the free list.
  my_add_to_free_list(metadata);
}

// This is called at the end of each challenge.
void my_finalize() {
  // Nothing is here for now.
  // feel free to add something if you want!
}

void test() {
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}
