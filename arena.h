/**
 * MIT License
 * Copyright (c) 2024 Baptiste ASSELIN
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * 
 * This code is widely inspired of this project : https://github.com/tsoding/arena
 * made by https://github.com/tsoding
*/

#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>
#include <string.h>

#define DEFAULT_ALLOC_SIZE 2048

typedef struct region_t {
    size_t size;
    size_t capacity;
    struct region_t* next;
    char data[];
} region_t;

typedef struct arena_t {
    region_t* first;
    struct arena_t* parent;
} arena_t;

void arena_free(arena_t* ctx);
void* arena_malloc(arena_t* ctx, size_t size);
void* arena_calloc(arena_t* ctx, size_t nmemb, size_t size);
void* arena_realloc(arena_t* ctx, void* ptr, size_t oldsize, size_t size);
char* arena_strdup(arena_t* ctx, const char* s);

#define DA_INIT_CAPACITY 10
#define arena_da_append(ctx, array, item) do { \
    if((array)->count >= (array)->capacity) { \
        if((array)->capacity == 0) { \
            (array)->items = arena_malloc((ctx), DA_INIT_CAPACITY*sizeof((array)->items[0])); \
            (array)->capacity = DA_INIT_CAPACITY; \
        } else {\
            (array)->items = arena_realloc((ctx), (array)->items, (array)->capacity*sizeof((array)->items[0]), (array)->capacity*2*sizeof((array)->items[0])); \
            (array)->capacity *= 2; \
        } \
    } \
    (array)->items[(array)->count] = (item); \
    (array)->count++; \
} while(0) \

#ifdef ARENA_IMPLEMENTATION

size_t space_in_region(region_t* region) {
    if(region == NULL) return -1;
    return region->capacity - region->size;
}

region_t* allocate_region(size_t size) {
    size_t allocated_size = (size < DEFAULT_ALLOC_SIZE) ? DEFAULT_ALLOC_SIZE : size;

    region_t* region = (region_t*)malloc(sizeof(region_t) + allocated_size);
    region->capacity = allocated_size;
    region->size = size;
    region->next = NULL;
    return region;
}

void arena_free(arena_t* ctx) {
    if(ctx == NULL) return;
    region_t* region = ctx->first;
    while(region != NULL) {
        region_t* current_region = region;
        region = region->next;
        free(current_region);
    }
    ctx->first = NULL;
}

void* arena_malloc(arena_t* ctx, size_t size) {
    if(ctx == NULL || size == 0) return NULL;

    if(ctx->first == NULL) {
        ctx->first = allocate_region(size);
        return (void*)ctx->first->data;
    }

    region_t* current_region = ctx->first;
    region_t* previous_region = NULL;

    while(current_region != NULL) {
        if(space_in_region(current_region) >= size) {
            void* ptr = &current_region->data[current_region->size];
            current_region->size += size;
            return ptr;
        }
        previous_region = current_region;
        current_region = current_region->next;
    }

    previous_region->next = allocate_region(size);
    return (void*)previous_region->next->data;
}

void* arena_calloc(arena_t* ctx, size_t nmemb, size_t size) {
    if(ctx == NULL || size == 0 || nmemb == 0) return NULL;

    void* ptr = arena_malloc(ctx, nmemb*size);
    memset(ptr, 0, nmemb*size);
    
    return ptr;
}

void* arena_realloc(arena_t* ctx, void* oldptr, size_t oldsize, size_t size) {
    if(ctx == NULL || oldptr == NULL || size == 0) return NULL;
    if(oldsize > size) return oldptr;

    void* newptr = arena_malloc(ctx, size);
    memcpy(newptr, oldptr, oldsize);
    return newptr;
}

char* arena_strdup(arena_t* ctx, const char* s) {
    if(s == NULL || ctx == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char* new_str = (char*)arena_calloc(ctx, len, sizeof(char));
    if (new_str == NULL) return NULL;
    memcpy(new_str, s, len);
    return new_str;
}

#endif // ARENA_IMPLEMENTATION
#endif // ARENA_H