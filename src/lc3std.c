#include "lc3std.h"

int stricmp(const char *lhs, const char *rhs) {
	while (true) {
		char clhs = *lhs++;
		char crhs = *rhs++;
		int diff = tolower(crhs) - tolower(clhs);
		if (diff != 0 || !clhs) {
			return diff;
		}
	}
	return 0;
}
int strnicmp(const char *lhs, const char *rhs, size_t maxlen) {
	for (; maxlen > 0; --maxlen) {
		char clhs = *lhs++;
		char crhs = *rhs++;
		int diff = tolower(crhs) - tolower(clhs);
		if (diff != 0 || !clhs) {
			return diff;
		}
	}
	return 0;
}

