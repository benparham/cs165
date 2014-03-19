#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bitmap.h>
#include <error.h>
#include <serial.h>

#define WORD_TYPE			unsigned char
#define WORD_MAX			-1
#define BITS_PER_WORD		8

struct bitmap {
	unsigned int nbits;
	int nWords;
	WORD_TYPE *map;
} bitmap;

static int wordsNeeded(int nBits) {
	int nWords = nBits / BITS_PER_WORD;
	if (nBits % BITS_PER_WORD > 0) {
		nWords += 1;
	}

	return nWords;
}

// Allocates a bitmap based on nbits desired and returns it
int bitmapCreate(unsigned int nbits, struct bitmap **bmp, error *err) {

	// Allocate the bitmap struct
	*bmp = (struct bitmap *) malloc(sizeof(struct bitmap));
	if (*bmp == NULL) {
		ERROR(err, E_NOMEM);
		goto exit;
	}

	// Set nbits and nWords
	(*bmp)->nbits = nbits;
	(*bmp)->nWords = wordsNeeded(nbits);

	int mapBytes = (*bmp)->nWords * sizeof(WORD_TYPE);

	// Allocate the words for the actual map
	(*bmp)->map = (WORD_TYPE *) malloc(mapBytes);
	if ((*bmp)->map == NULL) {
		ERROR(err, E_NOMEM);
		goto cleanupBitmap;
	}

	// Zero bits
	memset((*bmp)->map, 0, mapBytes);

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

void bitmapPrint(struct bitmap *bmp) {

	int size = bitmapSize(bmp);
	printf("Size: %d\n", size);
	printf("Words: %d\n", bmp->nWords);
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
	int mapBytes = bmp->nWords * sizeof(WORD_TYPE);

	memset(bmp->map, WORD_MAX, mapBytes);
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

int bitmapAddBits(struct bitmap *bmp, int addBits) {
	if (bmp == NULL) {
		return 1;
	}

	int newBits = bmp->nbits + addBits;
	int newWords = wordsNeeded(newBits);

	int oldBytes = bmp->nWords * sizeof(WORD_TYPE);
	int newBytes = newWords * sizeof(WORD_TYPE);

	if (newWords > bmp->nWords) {
		WORD_TYPE *temp = (WORD_TYPE *) realloc(bmp->map, newBytes);
		if (temp == NULL) {
			return 1;
		}

		bmp->map = temp;
		bmp->nWords = newWords;

		memset(bmp->map + oldBytes, 0, newBytes - oldBytes);
	}

	bmp->nbits = newBits;

	return 0;
}


// ============== Seriailization Functions

void bitmapSerialAddSize(serializer *slzr, struct bitmap *bmp) {
	int mapBytes = bmp->nWords * sizeof(WORD_TYPE);

	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeInt(slzr);
	serialAddSerialSizeRaw(slzr, mapBytes);
}

void bitmapSerialWrite(serializer *slzr, struct bitmap *bmp) {
	int mapBytes = bmp->nWords * sizeof(WORD_TYPE);

	serialWriteInt(slzr, bmp->nbits);
	serialWriteInt(slzr, bmp->nWords);
	serialWriteRaw(slzr, bmp->map, mapBytes);
}

void bitmapSerialRead(serializer *slzr, struct bitmap **bmp) {

	int nbits;
	int nWords;
	
	int mapBytes;
	WORD_TYPE *map;

	serialReadInt(slzr, &nbits);
	serialReadInt(slzr, &nWords);
	serialReadRaw(slzr, (void **) &map, &mapBytes);

	*bmp = (struct bitmap *) malloc(sizeof(struct bitmap));
	if (*bmp == NULL) {
		return;
	}

	(*bmp)->nbits = nbits;
	(*bmp)->nWords = nWords;
	(*bmp)->map = map;

}