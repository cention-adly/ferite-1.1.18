#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif

#include "ferite.h"

#include <stdio.h>
static unsigned int function_entry_counter = 0;
static unsigned int function_exit_counter = 0;
static unsigned int save_to_file_threshold = 100;

#define FERITE_PROFILE_NHASH 8192
struct ferite_profile_function_call *ferite_profile_function_calls[FERITE_PROFILE_NHASH];

static unsigned int hash(char *key)
{
	char *p = key;
	unsigned int hash = 0;
	while (*p) {
		hash = *p + 31 * hash;
		p++;
	}
	return hash % FERITE_PROFILE_NHASH;
}

static struct ferite_profile_function_call *hash_lookup(char *key) {
	unsigned int idx = hash(key);
	return ferite_profile_function_calls[idx];
}

#define DIE(reason) do { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, reason); exit(1); } while(0)

static void *xmalloc(size_t bytes)
{
	void *buf = malloc(bytes);
	if (buf == NULL)
		DIE("OOM");
	return buf;
}

static char *hash_get_key(char *filename, unsigned int line, char *funcname) {
	size_t len = strlen(filename) + strlen(funcname);
	char *key;

	while (line != 0) {
		line /= 10;
		len++;
	}
	len += 3; // filename|line|funcname\0
		//           ^    ^        ^

	key = xmalloc(len);
	snprintf(key, len, "%s|%d|%s", filename, line, funcname);
	return key;
}

static struct ferite_profile_function_call *ferite_profile_function_call_init(char *filename, int line, char *funcname)
{
	struct ferite_profile_function_call *fc;

	fc = xmalloc(sizeof(struct ferite_profile_function_call));
	fc->filename = ferite_strdup(filename, __FILE__, __LINE__);
	fc->function_name = ferite_strdup(funcname, __FILE__, __LINE__);
	fc->ncalls = 0;
	fc->total_duration.tv_sec = 0;
	fc->total_duration.tv_nsec = 0;
	fc->line = line;
	fc->head = fc->curr = fc->tail = fc->timings = NULL;
}

static int is_profile_for(char *filename, int line, char *funcname, struct ferite_profile_function_call *fc)
{
	// FIXME: The following code do not differentiate these two functions:
	//
	//	func foo return 1; func foo(number i) return 2;
	return fc->line == line &&
	       strcmp(fc->filename, filename) == 0 &&
	       strcmp(fc->function_name, funcname) == 0;
}

static struct ferite_profile_function_call *hash_get_or_create(char *filename, int line, char *funcname)
{
	struct ferite_profile_function_call *p = NULL, *tail;
	char *key = hash_get_key(filename, line, funcname);
	unsigned int idx = hash(key);
	struct ferite_profile_function_call *fc = ferite_profile_function_calls[idx];

	if (fc == NULL) {
		ferite_profile_function_calls[idx] = ferite_profile_function_call_init(filename, line, funcname);
		return ferite_profile_function_calls[idx];
	} else {
		p = fc;
		while (p) {
			if (is_profile_for(filename, line, funcname, fc))
				return fc;
			tail = p;
			p = p->next;
		}
	}

	tail = ferite_profile_function_call_init(filename, line, funcname);

	return tail;
}

void ferite_trace_init()
{
	fprintf(stderr, "ohai ferite_init\n");
}

static void save_trace_data()
{
	int i;

	ferite_trace_record();
}

static struct ferite_profile_timings *ferite_profile_timings_create()
{
	struct ferite_profile_timings *t = xmalloc(sizeof(struct ferite_profile_timings));
	t->next = t->prev = NULL;
	bzero(&t->end, sizeof(struct timespec));
	return t;
}

void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name)
{
	struct ferite_profile_function_call *fc = hash_get_or_create(file, line, name);

	if (fc->timings == NULL) {
		fc->head = fc->curr = fc->tail = fc->timings = ferite_profile_timings_create();
	} else {
		if (fc->curr->next == NULL) {
			fc->curr->next = ferite_profile_timings_create();
			fc->curr->next->prev = fc->curr;
		}
		fc->curr = fc->tail = fc->curr->next;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &fc->curr->begin);
	fc->ncalls++;

	function_entry_counter++;
}

void ferite_trace_function_exit(int level, char *file, unsigned int line, char *name)
{
	struct ferite_profile_function_call *fc = hash_get_or_create(file, line, name);

	if (fc->timings == NULL)
		DIE("Shouldn't happen!");

	clock_gettime(CLOCK_MONOTONIC_RAW, &fc->curr->end);
	fc->total_duration.tv_nsec += fc->curr->end.tv_nsec;
	fc->total_duration.tv_sec += fc->total_duration.tv_nsec / 1000000000;
	fc->total_duration.tv_nsec = fc->total_duration.tv_nsec % 1000000000;

	fc->curr = fc->tail = fc->curr->prev;

	function_exit_counter++;

	if (function_exit_counter % save_to_file_threshold == 0) {
		save_trace_data();
	}
}

void ferite_trace_record()
{
	fprintf(stderr, "Total function_entry_counter: %d, exit: %d\n", function_entry_counter, function_exit_counter);
}
