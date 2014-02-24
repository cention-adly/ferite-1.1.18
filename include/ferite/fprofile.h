#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__ 1

#include "time.h"

struct ferite_profile_timestamps {
	struct timespec tv;
	struct ferite_profile_timestamps *next;
};

struct ferite_profile_Trie {
	int isTerminal;
	char letter;
	struct ferite_profile_Trie *children[128];
	struct ferite_profile_timestamps *enter;
	struct ferite_profile_timestamps *exit;
};

void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_exit(int level, char *file, unsigned int line, char *name);
void ferite_trace_record();
#endif
