#include <stdio.h>

#include <insert.h>
#include <table.h>
#include <column.h>
#include <error.h>
#include <command.h>

int sorted_insert(columnBuf *colBuf, error *err) {


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

	

	columnBufDestroy(colBuf);



	// columnBuf *colBuf = fetchCol(tbl, columnName, err);
	// if (colBuf == NULL) {
	// 	printf("Col Buf was null\n");
	// 	return 1;
	// }

	// int result = 0;

	// printColumnInfo(&(colBuf->colInfo));

	// switch (colBuf->colInfo.storageType) {
	// 	case COL_UNSORTED:
	// 		result = sorted_insert(colBuf, err);
	// 		break;
	// 	default:
	// 		err->err = ERR_INTERNAL;
	// 		err->message = "Column sort type unsupported";
	// 		result = 1;
	// 		break;
	// }


	// pthread_mutex_unlock(&(colBuf->colLock));

	return 0;

cleanupColBuf:
	columnBufDestroy(colBuf);
exit:
	return 1;
}