#include <string.h>

#include <columnTypes/common.h>

int strToColStorage(char *str, COL_STORAGE_TYPE *type) {
	int result = 1;

	if (strcmp(str, "unsorted") == 0) {
		*type = COL_UNSORTED;
		result = 0;
	} else if (strcmp(str, "sorted") == 0) {
		*type = COL_SORTED;
		result = 0;
	} else if (strcmp(str, "btree") == 0) {
		*type = COL_BTREE;
		result = 0;
	}

	return result;
}

int seekHeader(FILE *fp, int offset, error *err) {

	if (fseek(fp, sizeof(COL_STORAGE_TYPE) + offset, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	return 0;
}

int commonFetch(int headerSizeBytes, FILE *fp, struct bitmap *bmp, int *resultBytes, int **results, error *err) {

	// Seek to data
	if (seekHeader(fp, headerSizeBytes, err)) {
		return 1;
	}

	int resultBuf[BUFSIZE];
	int resultOffset = 0;

	int length = bitmapSize(bmp);
	for (int i = 0; i < length; i++) {
		if (resultOffset >= BUFSIZE) {
			ERROR(err, E_NOMEM);
			return 1;
		}

		if (bitmapIsSet(bmp, i)) {
			int entry;
			if (fread(&entry, sizeof(int), 1, fp) < 1) {
				ERROR(err, E_FRD);
				return 1;
			}

			resultBuf[resultOffset] = entry;
			resultOffset += 1;

		} else {
			if (fseek(fp, sizeof(int), SEEK_CUR) == -1) {
				ERROR(err, E_FSK);
				return 1;
			}
		}
	}

	*resultBytes = resultOffset * sizeof(int);

	*results = (int *) malloc(*resultBytes);
	if (*results == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}
	memcpy(*results, resultBuf, *resultBytes);
	
	return 0;
}