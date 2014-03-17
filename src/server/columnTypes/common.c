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
		// err->err = ERR_INTERNAL;
		// err->message = "Failed to seek in column file";
		ERROR(err, E_FSK);
		return 1;
	}

	return 0;
}

int commonFetch(int headerSizeBytes, FILE *fp, struct bitmap *bmp, error *err) {

	// Seek to data
	if (seekHeader(fp, headerSizeBytes, err)) {
		return 1;
	}

	printf("Fetch results:\n");

	int length = bitmapSize(bmp);
	for (int i = 0; i < length; i++) {
		if (bitmapIsSet(bmp, i)) {
			int entry;
			if (fread(&entry, sizeof(int), 1, fp) < 1) {
				// err->err = ERR_INTERNAL;
				// err->message = "Failed to read from column file";
				ERROR(err, E_FRD);
				return 1;
			}

			printf("%d,", entry);

		} else {
			if (fseek(fp, sizeof(int), SEEK_CUR) == -1) {
				// err->err = ERR_INTERNAL;
				// err->message = "Failed to seek in column file";
				ERROR(err, E_FSK);
				return 1;
			}
		}
	}

	return 0;
}