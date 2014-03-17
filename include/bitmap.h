/*
 * Based off code written by David A. Holland for OS161. Thanks David!
 */

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <error.h>

struct bitmap;

int bitmapCreate(unsigned int nbits, struct bitmap **bmp, error *err);
void bitmapDestroy(struct bitmap *bmp);

int bitmapSize(struct bitmap *bmp);

void bitmapPrint(struct bitmap *bmp, error *err);

int bitmapMark(struct bitmap *bmp, int idx, error *err);
void bitmapMarkAll(struct bitmap *bmp);
int bitmapUnmark(struct bitmap *bmp, int idx, error *err);
int bitmapIsSet(struct bitmap *bmp, int idx);


#endif