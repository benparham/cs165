#include <stdlib.h>
#include <string.h>

#include <bitmap.h>

#define WORD_TYPE		unsigned char
#define BITS_PER_WORD	8

struct bitmap {
	unsigned int nbits;
	WORD_TYPE *map;
} bitmap;

// Allocates a bitmap based on nbits desired and returns it
struct bitmap* bitmapCreate(unsigned int nbits) {
	struct bitmap *bmp;
	unsigned int words;

	// Get number of words needed to store n bits
	words = nbits / BITS_PER_WORD;
	if (nbits % BITS_PER_WORD > 0) {
		words += 1;
	}

	// Allocate the bitmap struct
	bmp = (struct bitmap *) malloc(sizeof(struct bitmap));
	if (bmp == NULL) {
		goto exit;
	}

	// Allocate the words for the actual map
	bmp->map = (WORD_TYPE *) malloc(sizeof(WORD_TYPE) * words);
	if (bmp->map == NULL) {
		goto cleanupBitmap;
	}

	// Zero bits
	memset(bmp->map, 0, sizeof(WORD_TYPE) * words);

	// Set the nbits value
	bmp->nbits = nbits;

	return bmp;

// cleanupSubMap:
// 	free(bmp->map);
cleanupBitmap:
	free(bmp);
exit:
	return NULL;
}

void bitmapDestroy(struct bitmap *bmp) {
	free(bmp->map);
	free(bmp);
}

int bitmapSize(struct bitmap *bmp) {
	return bmp->nbits;
}

static void bitmapTranslate(unsigned int bitNum, unsigned int *wordIdx, WORD_TYPE *mask) {
	*wordIdx = bitNum / BITS_PER_WORD;
	
	unsigned int offset = bitNum % BITS_PER_WORD;
	*mask = ((WORD_TYPE) 1) << offset;
}

int bitmapMark(struct bitmap *bmp, int idx) {
	unsigned int wordIdx;
	WORD_TYPE mask;

	if (idx >= bmp->nbits || idx < 0) {
		return 1;
	}

	bitmapTranslate(idx, &wordIdx, &mask);

	bmp->map[wordIdx] |= mask;

	return 0;
}

int bitmapUnmark(struct bitmap *bmp, int idx) {

	unsigned int wordIdx;
	WORD_TYPE mask;

	if (idx >= bmp->nbits || idx < 0) {
		return 1;
	}

	bitmapTranslate(idx, &wordIdx, &mask);

	bmp->map[wordIdx] &= ~mask;

	return 0;

}

int bitmapIsSet(struct bitmap *bmp, int idx) {

	unsigned int wordIdx;
	WORD_TYPE mask;

	if (idx >= bmp->nbits || idx < 0) {
		return 1;
	}

	bitmapTranslate(idx, &wordIdx, &mask);

	return (bmp->map[wordIdx] & mask);

}