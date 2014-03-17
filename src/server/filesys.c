#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <filesys.h>
#include <error.h>
#include <global.h>


int fileExists(char *pathToFile) {
	struct stat st;
	return (stat(pathToFile, &st) == 0);
}

int dirExists(char *pathToDir) {
	struct stat st;
	if (stat(pathToDir, &st) != 0) {
		return 0;
	}
	return S_ISDIR(st.st_mode);
}

int removeDir(char *pathToDir, error *err) {
	DIR *dp = opendir(pathToDir);
	if (dp == NULL) {
		ERROR(err, E_DOP);
		return 1;
	}

	char pathToElement[BUFSIZE];
	struct dirent *ep;
	while ((ep = readdir(dp)) != NULL) {
		if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
			continue;
		}

		sprintf(pathToElement, "%s/%s", pathToDir, ep->d_name);

		if (dirExists(pathToElement)) {
			removeDir(pathToElement, err);
		} else {
			if (remove(pathToElement) != 0) {
				ERROR(err, E_FRM);

				closedir(dp);
				return 1;
			}
		}
	}

	closedir(dp);
	if (rmdir(pathToDir) != 0) {
		ERROR(err, E_DRM);
		return 1;
	}

	return 0;
}