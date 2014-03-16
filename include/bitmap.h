/*
 * Based off code written by David A. Holland for OS161. Thanks David!
 */

#ifndef _BITMAP_H_
#define _BITMAP_H_

struct bitmap;

int bitmapCreate(unsigned int nbits, struct bitmap **bmp);
void bitmapDestroy(struct bitmap *bmp);

int bitmapSize(struct bitmap *bmp);

void bitmapPrint(struct bitmap *bmp);

int bitmapMark(struct bitmap *bmp, int idx);
int bitmapMarkAll(struct bitmap *bmp);
int bitmapUnmark(struct bitmap *bmp, int idx);
int bitmapIsSet(struct bitmap *bmp, int idx);


#endif