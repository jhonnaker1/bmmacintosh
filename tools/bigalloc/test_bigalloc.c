/*
 * test_bigalloc.c - tests src/port/bigalloc.c on the PC.
 *
 * The __real_ functions here behave like Circle's heap: a freed block
 * bigger than 512K is lost (counted in LOST and never given again), and
 * realloc() gives back the same block when the new size fits. The test
 * repeats BMMacintosh's Mac II and Mac Plus restarts (their memory,
 * Basilisk II's tables, the screen and the display's new[] buffers) and
 * growing realloc() buffers many times, and checks that the memory taken from the heap stays
 * what the first round took.
 *
 * Run: ./run_tests.sh
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIG 0x80000

void *__wrap_malloc(size_t size);
void __wrap_free(void *block);
void *__wrap_calloc(size_t count, size_t size);
void *__wrap_realloc(void *block, size_t size);

static size_t taken, lost;

typedef struct { size_t size; size_t pad; } header;

void *__real_malloc(size_t size)
{
	header *h = malloc(sizeof *h + size);
	h->size = size;
	taken += size;
	memset(h + 1, 0xa5, size); /* not zero, so calloc must clear */
	return h + 1;
}

void __real_free(void *block)
{
	header *h = (header *)block - 1;
	if (block == NULL)
		return;
	if (h->size > BIG)
		lost += h->size; /* Circle never reuses it (keep it allocated) */
	else {
		taken -= h->size;
		free(h);
	}
}

size_t bigalloc_block_size(const void *block)
{
	return ((const header *)block - 1)->size;
}

void *__real_realloc(void *block, size_t size)
{
	header *h = (header *)block - 1;
	void *n;
	if (h->size >= size)
		return block;
	n = __real_malloc(size);
	memcpy(n, block, h->size);
	__real_free(block);
	return n;
}

/* C++'s new[] and delete[] as wrapped (bigalloc.c). */
#if __SIZEOF_SIZE_T__ == 8
#define CXX_SIZE "m"
#else
#define CXX_SIZE "j"
#endif
void *bigalloc_new_array(size_t size) __asm__("__wrap__Zna" CXX_SIZE);
void bigalloc_delete_array(void *block) __asm__("__wrap__ZdaPv");

#define MB (1024u * 1024)

/* One start of a Mac II with RAM_MB of RAM (start_mac() in
   src/port/ii/main_ii.cpp): its memory block (RAM, 1 MB of ROM, scratch,
   the 1600x1200 frame buffer), Basilisk's 68k table and the screen's
   shadow, each freed and allocated again; and the Pi 4/5 display's
   picture and CRT tables made again with new[] (src/av). */
static void mac_ii_start(void **mem, void **table, void **shadow, void **pic,
                         void **index, unsigned ram_mb, int round)
{
	const size_t fb = 1600 * 1200 * 4;
	__wrap_free(*mem);
	*mem = __wrap_calloc(1, ram_mb * MB + MB + 0x10000 + fb + 4096);
	__wrap_free(*table);
	*table = __wrap_malloc(65536 * 40);
	__wrap_free(*shadow);
	*shadow = __wrap_malloc(fb);
	bigalloc_delete_array(*pic);
	*pic = bigalloc_new_array((size_t)(round & 1 ? 1024 * 768 : 1280 * 1024) * 2);
	bigalloc_delete_array(*index);
	*index = bigalloc_new_array((size_t)1920 * 1080 * 4);
	assert(*mem && *table && *shadow && *pic && *index);
	memset(*mem, round, 4096);
}

int main(void)
{
	void *mem = NULL, *table = NULL, *shadow = NULL, *pic = NULL, *index = NULL;
	void *plus = NULL, *buf;
	size_t after_first = 0;
	int i;

	/* Restarts of a 128 MB Mac II, switches to the Mac Plus's 4 MB and
	   back: after the first few rounds the heap must not grow any more. */
	for (i = 0; i < 1000; i++) {
		if (i == 10)
			after_first = taken + lost;
		mac_ii_start(&mem, &table, &shadow, &pic, &index, 128, i);
		__wrap_free(plus);
		plus = __wrap_calloc(1, 4 * MB);
		assert(((unsigned char *)plus)[4 * MB - 1] == 0);
	}
	assert(taken + lost == after_first);
	assert(taken + lost < 200u * MB);

	/* Changing the Mac II's memory to 256 MB and back costs one block of
	   each size, not one per restart. */
	for (i = 0; i < 100; i++)
		mac_ii_start(&mem, &table, &shadow, &pic, &index, i & 1 ? 256 : 128, i);
	assert(taken + lost < after_first + 280u * MB);

	/* calloc() clears a reused block. */
	buf = __wrap_calloc(1, 2 * 1024 * 1024);
	for (i = 0; i < 2 * 1024 * 1024; i++)
		assert(((unsigned char *)buf)[i] == 0);
	memset(buf, 7, 2 * 1024 * 1024);
	__wrap_free(buf);
	buf = __wrap_calloc(2 * 1024, 1024);
	assert(((unsigned char *)buf)[12345] == 0);
	__wrap_free(buf);

	/* realloc() growing a small block into a big one,
	   then bigger, keeps its contents and loses nothing once freed. */
	size_t before = taken + lost;
	for (i = 0; i < 100; i++) {
		buf = __wrap_realloc(NULL, 1000);
		memset(buf, 0x3c, 1000);
		buf = __wrap_realloc(buf, 600000);
		assert(((unsigned char *)buf)[999] == 0x3c);
		memset(buf, 0x5a, 600000);
		buf = __wrap_realloc(buf, 3 * 1024 * 1024);
		assert(((unsigned char *)buf)[599999] == 0x5a);
		buf = __wrap_realloc(buf, 100); /* shrinking keeps it */
		__wrap_free(buf);
	}
	assert(taken + lost <= before + 600000 + 3 * 1024 * 1024);
	assert(__wrap_realloc(__wrap_malloc(10), 0) == NULL);

	/* Small blocks go straight to the heap and back. */
	size_t small = taken;
	buf = __wrap_malloc(1000);
	__wrap_free(buf);
	assert(taken == small);

	printf("bigalloc: ok (heap used %zu MB, lost %zu MB after 1100 restarts)\n",
	       (taken + lost) >> 20, lost >> 20);
	return 0;
}
