#include <stdio.h>
#include <string.h>

#include <insert.h>
#include <table.h>
#include <column.h>
#include <error.h>
#include <command.h>

static int convertDataByType(COL_DATA_TYPE type, char *inData, void *outData, int *sizeBytes) {
	switch (type) {
		case COL_INT:
			*((int *) outData) = atoi(inData);
			*sizeBytes = sizeof(int);

			printf("In data: %s\n", inData);
			printf("Out data: %d\n", *((int *) outData));

			break;
		default:
			return 1;
	}

	return 0;
}

static int insertUnsorted(columnBuf *colBuf, char *data, error *err) {
	
	colDataType toWrite;
	int sizeBytes;

	if (convertDataByType(colBuf->colInfo.dataType, data, (void *) &toWrite, &sizeBytes)) {
		err->err = ERR_INTERNAL;
		err->message = "Data type unsupported for insertion";
		return 1;
	}

	// Write data to end of file
	if (fseek(colBuf->fp, 0, SEEK_END) == -1) {
		return 1;
	}
	if (fwrite(&toWrite, sizeBytes, 1, colBuf->fp) <= 0) {
		err->err = ERR_INTERNAL;
		err->message = "Unable to write data to column file";
		return 1;
	}

	return 0;
}


int insert(tableInfo *tbl, insertArgs *args, error *err) {
	char *columnName = args->columnName;
	printf("Inserting into column '%s'...\n", columnName);

	columnBuf *colBuf;
	if (columnBufCreate(&colBuf)) {
		err->err = ERR_INTERNAL;
		err->message = "Cannot create column buffer. Out of memory.";
		goto exit;
	}

	if (getColumnFromDisk(tbl, columnName, "rb+", colBuf, err)) {
		goto cleanupColBuf;
	}

	printf("Got column '%s' from disk:\n", columnName);
	printColumnInfo(&(colBuf->colInfo));

	switch(colBuf->colInfo.storageType) {
		case COL_UNSORTED:
			if (insertUnsorted(colBuf, args->value, err)) {
				goto cleanupColBuf;
			}
			break;
		case COL_SORTED:
			// break;
		case COL_BTREE:
			// break;
		default:
			err->err = ERR_INTERNAL;
			err->message = "Invalid column storage type";
			goto cleanupColBuf;
			break;
	}

	fclose(colBuf->fp);
	columnBufDestroy(colBuf);

	printf("Inserted into column '%s'\n", columnName);

	return 0;

cleanupColBuf:
	fclose(colBuf->fp);
	columnBufDestroy(colBuf);
exit:
	return 1;
}