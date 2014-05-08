#include <string.h>

#include <columnTypes/common.h>

int strToColStorage(char *str, COL_STORAGE_TYPE *type) {
	int result = 1;

	if (strcmp(str, "\"unsorted\"") == 0) {
		*type = COL_UNSORTED;
		result = 0;
	} else if (strcmp(str, "\"sorted\"") == 0) {
		*type = COL_SORTED;
		result = 0;
	} else if (strcmp(str, "\"b+tree\"") == 0) {
		*type = COL_BTREE;
		result = 0;
	}

	return result;
}

int seekHeader(FILE *headerFp, error *err) {

	if (fseek(headerFp, sizeof(COL_STORAGE_TYPE), SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	return 0;
}



int commonFetch(FILE *dataFp, struct bitmap *bmp, int *resultBytes, int **results, int **indices, error *err) {

	// Seek to beginning of file
	if (fseek(dataFp, 0, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	int resultBuf[BUFSIZE];
	int indexBuf[BUFSIZE];
	int resultOffset = 0;

	int length = bitmapSize(bmp);
	for (int i = 0; i < length; i++) {
		if (resultOffset >= BUFSIZE) {
			ERROR(err, E_NOMEM);
			return 1;
		}

		if (bitmapIsSet(bmp, i)) {
			int entry;
			if (fread(&entry, sizeof(int), 1, dataFp) < 1) {
				ERROR(err, E_FRD);
				return 1;
			}

			resultBuf[resultOffset] = entry;
			indexBuf[resultOffset] = i;
			resultOffset += 1;

		} else {
			if (fseek(dataFp, sizeof(int), SEEK_CUR) == -1) {
				ERROR(err, E_FSK);
				return 1;
			}
		}
	}

	*resultBytes = resultOffset * sizeof(int);

	*results = (int *) malloc(*resultBytes);
	*indices = (int *) malloc(*resultBytes);
	if (*results == NULL) {
		ERROR(err, E_NOMEM);
		return 1;
	}
	if (*indices == NULL) {
		free(*results);
		ERROR(err, E_NOMEM);
		return 1;
	}
	memcpy(*results, resultBuf, *resultBytes);
	memcpy(*indices, indexBuf, *resultBytes);
	
	return 0;
}

int commonLoad(FILE *dataFp, int dataBytes, int *data, error *err) {

	// Seek to beginning of file
	if (fseek(dataFp, 0, SEEK_SET) == -1) {
		ERROR(err, E_FSK);
		return 1;
	}

	// Write from data into the file
	if (fwrite(data, dataBytes, 1, dataFp) < 1) {
		ERROR(err, E_FWR);
		return 1;
	}

	return 0;
}