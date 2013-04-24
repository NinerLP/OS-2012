#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define MIN_SIZE 32
#define MAX_SIZE 65536

typedef struct _LinearBuffer {
	void *mem; //pointer to buffer
	int totalSize;
	int offset;
} LinearBuffer;

void * largePtrs[MAX_LARGE] = {-1};

void * linearBufferMalloc(LinearBuffer* buf, size_t) {
	if (!buf || !size) {
		return NULL;
	}
	int newOffset = buf->offset + MIN_SIZE;
	if (newOffset <= buf->totalSize) {
		void * ptr = buf->mem + buf->offset;
		buf->offset = newOffset;
		return ptr;
	}
	return NULL; //oom
}

void * linearBufferFree(LinearBuffer* buf) {
	//is it supposed to do anything?, we can clear linear buffer by setting offset to 0 though.
}

void * malloc_dumb(size_t size) {
	void * ptr = mmap(NULL, size+sizeof(size_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
		return NULL;
	}
	size_t * size_ptr = (size_t *) ptr;
	*(size_ptr) = size + sizeof(size_t);
	return (void *) (size_ptr+sizeof(size_t));
}

void free_dumb(void * ptr) {
	if (ptr == NULL) {
		return;
	}
	size_t * size_ptr = ((size_t *) ptr) - sizeof(size_t);
	munmap((void *) size_ptr, *size_ptr);
}

void * malloc(size_t size) {
	//let's save size regardless of allocation size
	//allocated memory will always be |size|memory........|
	size_t realsize = size + sizeof(size_t);
	void * ptr;
	//we allocate that memory
	if (realsize < MIN_SIZE) {
		//linear allocation with buckets
	} else if (realsize < MAX_SIZE) {
		//clever allocation with heap
	} else {
		//dumb allocation with malloc_dumb
		ptr = mmap(NULL, realsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (ptr == MAP_FAILED) {
			return NULL;
		}
	}
	//in the end we record size and return ptr
	size_t * size_ptr = (size_t *) ptr;
	*(size_ptr) = realsize;
	return (void *) size_ptr + sizeof(size_t);
}

void free(void * ptr) {
	if (ptr == NULL) {
		return;
	}
	//we get realsize that was calculated
	size_t * size_ptr = ((size_t *) ptr) - sizeof(size_t);
	realsize = *size_ptr;
	if (realsize < MIN_SIZE) {
		//we used linear allocation, is there supposed to be "free" for it?
	} else if (realsize < MAX_SIZE) {
		//heap thingy free blahblah
	} else {
		//dumbfree
		munmap((void *) size_ptr, *size_ptr);
	}	
}
