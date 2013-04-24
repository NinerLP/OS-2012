#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define MIN_SIZE 32
#define MAX_SIZE 65536
#define SML_LIMIT 20

typedef struct _bucket {
	size_t len;
	pid_t tid;
	void * mem;	
	struct _bucket* next;
} bucket;

typedef struct _localthread {
	pid_t tid;
	pthread_mutex_t small_bucket_mutex;
	bucket* small_list;
	struct _localthread* next;
} localthread;

static pthread_mutex_t thread_list_mutex;
static pthread_mutex_t memory_list_mutex;
static localthread* thread_list = NULL;
static bucket* bucket_list = NULL;

static bucket* new_bucket(size_t size) {
	bucket* res = (bucket *)mmap(NULL, sizeof(bucket)+size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (res) {
		res->len = size;
		res->mem = (void*) res +sizeof(bucket);
		res->next = NULL;
		res->tid = pthread_self();
	}
	return res;
}

static void free_bucket(bucket* buck) {
	if (buck == NULL) {
		return;
	}
	free_bucket(buck->next);
	munmap((void *) buck,sizeof(bucket)+buck->len);
}

static void free_localthread(localthread* lt) {
	if (lt == NULL) {
		return;
	}
	free_bucket(lt->small_list);
	pthread_mutex_destroy(&(lt->small_bucket_mutex));
	free_localthread(lt->next);
	munmap((void *)lt, sizeof(localthread));
}

static localthread* get_localthread(pid_t tid) {
	localthread* cur = thread_list;
	while (cur) {
		if (cur->tid == tid) break;
		cur = cur->next;
	}
	if (cur == NULL) {
		cur = (localthread*) mmap(NULL, sizeof(localthread), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		cur->tid = tid;
		cur->small_list = NULL;
		pthread_mutex_init(&(cur->small_bucket_mutex), 0);
		cur->next = thread_list;
		thread_list = cur;
	}
	return cur;
}

static size_t bucket_list_size(bucket* buck){
	size_t cnt;
	for (cnt = 0; buck != NULL; cnt++, buck = buck->next);
	return cnt;
}

void * malloc(size_t size) {
	pthread_mutex_lock(&thread_list_mutex);
	localthread* lt = get_localthread(pthread_self());
	pthread_mutex_unlock(&thread_list_mutex);
	if (lt == NULL) {
		return NULL;
	}
	//let's save size regardless of allocation size
	//allocated memory will always be |size|memory........|
	size_t realsize = size + sizeof(size_t);
	void * ptr;
	//we allocate that memory
	if (realsize < MIN_SIZE) {
		//linear allocation with buckets
		bucket* res;
		pthread_mutex_lock(&(lt->small_bucket_mutex));
		if (lt->small_list) {
			res = lt->small_list;
			lt->small_list = res->next;
			res->next = NULL;
		}	
		pthread_mutex_unlock(&(lt->small_bucket_mutex));	
		if (res) {
			return res->mem;
		}
		res = new_bucket(MIN_SIZE);	
		return res->mem;
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
	pthread_mutex_lock(&thread_list_mutex);
	localthread* lt = get_localthread(pthread_self());
	pthread_mutex_unlock(&thread_list_mutex);
	if (lt == NULL) {
		return;
	}
	//we get realsize that was calculated
	size_t * size_ptr = ((size_t *) ptr) - sizeof(size_t);
	size_t realsize = *size_ptr;
	if (realsize < MIN_SIZE) {
		//lin
		bucket* buck = (bucket*) (ptr-sizeof(bucket));
		pthread_mutex_lock(&(lt->small_bucket_mutex));
		if (bucket_list_size(lt->small_list) < SML_LIMIT) {
			buck->next = lt->small_list;
			lt->small_list = buck;
			buck = NULL;
		}
		pthread_mutex_unlock(&(lt->small_bucket_mutex));
		if (buck) {
			free_bucket(buck);
		}
	} else if (realsize < MAX_SIZE) {
		//heap thingy free blahblah
	} else {
		//dumbfree
		munmap((void *) size_ptr, *size_ptr);
	}	
}
