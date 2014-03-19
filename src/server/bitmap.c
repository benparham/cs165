#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bitmap.h>
#include <error.h>

#define WORD_TYPE		unsigned char
#define WORD_MAX		-1
#define BITS_PER_WORD	8

struct bitmap {
	unsigned int nbits;
	WORD_TYPE *map;
} bitmap;

// Allocates a bitmap based on nbits desired and returns it
int bitmapCreate(unsigned int nbits, struct bitmap **bmp, error *err) {
	unsigned int words;

	// Get number of words needed to store n bits
	words = nbits / BITS_PER_WORD;
	if (nbits % BITS_PER_WORD > 0) {
		words += 1;
	}

	// Allocate the bitmap struct
	*bmp = (struct bitmap *) malloc(sizeof(struct bitmap));
	if (*bmp == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	// Allocate the words for the actual map
	(*bmp)->map = (WORD_TYPE *) malloc(sizeof(WORD_TYPE) * words);
	if ((*bmp)->map == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupBitmap;
	}

	// Zero bits
	memset((*bmp)->map, 0, sizeof(WORD_TYPE) * words);

	// Set the nbits value
	(*bmp)->nbits = nbits;

	return 0;

cleanupBitmap:
	free(*bmp);
exit:
	return 1;
}

void bitmapDestroy(struct bitmap *bmp) {
	free(bmp->map);
	free(bmp);
}

int bitmapSize(struct bitmap *bmp) {
	return bmp->nbits;
}

void bitmapPrint(struct bitmap *bmp, error *err) {

	int size = bitmapSize(bmp);
	printf("Size: %d\n", size);
	printf("Map:\n[");
	for (int i = 0; i < size; i++) {
		printf("%d", bitmapIsSet(bmp, i) ? 1 : 0);
		if (i != size - 1) {
			printf(",");
		}
	}
	printf("]\n");
}

static void bitmapTranslate(unsigned int bitNum, unsigned int *wordIdx, WORD_TYPE *mask) {
	*wordIdx = bitNum / BITS_PER_WORD;
	
	unsigned int offset = bitNum % BITS_PER_WORD;
	*mask = ((WORD_TYPE) 1) << offset;
}

int bitmapMark(struct bitmap *bmp, int idx, error *err) {
	unsigned int wordIdx;
	WORD_TYPE mask;

	if (idx >= bmp->nbits || idx < 0) {
		ERROR(err, E_OUTRNG);
		return 1;
	}

	bitmapTranslate(idx, &wordIdx, &mask);

	bmp->map[wordIdx] |= mask;

	return 0;
}

void bitmapMarkAll(struct bitmap *bmp) {
	unsigned int nWords = bmp->nbits / BITS_PER_WORD;
	if (bmp->nbits % BITS_PER_WORD > 0) {
		nWords += 1;
	}

	memset(bmp->map, WORD_MAX, sizeof(WORD_TYPE) * nWords);
}

int bitmapUnmark(struct bitmap *bmp, int idx, error *err) {

	unsigned int wordIdx;
	WORD_TYPE mask;

	if (idx >= bmp->nbits || idx < 0) {
		ERROR(err, E_OUTRNG);
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
		return 0;
	}

	bitmapTranslate(idx, &wordIdx, &mask);

	return (bmp->map[wordIdx] & mask);

}

void bitmapSerialize(struct bitmap *bmp, void *serial, int *offset) {

	// int numWords = bmp->nbits / BITS_PER_WORD;
	// if (bmp->nbits % BITS_PER_WORD > 0) {
	// 	numWords += 1;
	// }

	// serialWrite(serial, offset, &(bmp->nbits), sizeof(unsigned int));
	// serialWrite(serial, offset, &numWords, sizeof(int));
	// serialWrite(serial, offset, bmp->map, numWords * sizeof(WORD_TYPE));
}

void bitmapDeserialize(struct bitmap **bmp, void *serial, int *offset) {

	// unsigned int nBits;
	// int numWords;

	// serialRead(serial, offset, &nBits, sizeof(unsigned int));
	// serialRead(serial, offset, &numWords, sizeof(int));
	

}