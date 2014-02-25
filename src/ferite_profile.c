#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif

#include <stdio.h>

#include "ferite.h"
#define ONE_BILLION 1000000000L

static unsigned int save_to_file_threshold = 100;

#define FERITE_PROFILE_NHASH 8192
#define FERITE_PROFILE_STACK_SIZE 20
static struct profile_entry *profile_entries[FERITE_PROFILE_NHASH];

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

static struct profile_entry *hash_lookup(char *key) {
	unsigned int idx = hash(key);
	return profile_entries[idx];
}

#define DIE(reason) do { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, reason); exit(1); } while(0)

static void *xmalloc(size_t bytes)
{
	void *buf = malloc(bytes);
	if (buf == NULL)
		DIE("OOM");
	return buf;
}

static char *hash_get_key(char *filename, unsigned int lnum) {
	size_t len = strlen(filename);
	int l = lnum;
	char *key;
	int ntruncated = 0;

	// calculate width of line number
	do {
		len += 1;
		l /= 10;
	} while(l > 0);
	len += 2; // filename:lnum\0
		  //         ^     ^
	key = xmalloc(len);
	if ((ntruncated = snprintf(key, len, "%s:%d", filename, lnum)) >= len) {
		fprintf(stderr, "FIXME: %d bytes for key '%s' truncated for %s:%d\n", ntruncated, key, filename, lnum);
	}
	return key;
}

static struct profile_entry *ferite_profile_init(char *filename, int line)
{
	struct profile_entry *pe;

	pe = xmalloc(sizeof(struct profile_entry));
	pe->filename = ferite_strdup(filename, __FILE__, __LINE__);
	pe->ncalls = 0;
	pe->total_duration.tv_sec = 0;
	pe->total_duration.tv_nsec = 0;
	pe->line = line;
	pe->stack = ferite_create_stack(NULL, FERITE_PROFILE_STACK_SIZE);

	pe->next = NULL;
}

static int is_profile_for(char *filename, int line, struct profile_entry *pe)
{
	return strcmp(pe->filename, filename) == 0 && pe->line == line;
}

static struct profile_entry *hash_get_or_create(char *filename, int line)
{
	struct profile_entry *p = NULL;
	char *key = hash_get_key(filename, line);
	unsigned int idx = hash(key);
	struct profile_entry *pe = profile_entries[idx], *tail;

	if (pe == NULL) {
		profile_entries[idx] = ferite_profile_init(filename, line);
		return profile_entries[idx];
	} else {
		p = pe;
		while (p) {
			if (is_profile_for(filename, line, pe))
				return pe;
			tail = p;
			p = p->next;
		}
	}

	tail->next = ferite_profile_init(filename, line);

	return tail;
}

void ferite_trace_init()
{
	int i;
	fprintf(stderr, "ohai ferite_init\n");
	bzero(profile_entries, sizeof(struct profile_entry *) * FERITE_PROFILE_NHASH);
}

static void save_trace_data()
{
	ferite_trace_record();
}

static struct timespec *create_timestamp()
{
	struct timespec *t = xmalloc(sizeof(struct timespec));

	clock_gettime(CLOCK_MONOTONIC_RAW, t);
	return t;
}

static struct timespec timespec_diff(struct timespec *old, struct timespec *new)
{
	struct timespec d;
	if (new->tv_nsec < old->tv_nsec) {
		new->tv_sec -= 1;
		new->tv_nsec += ONE_BILLION;
	}
	d.tv_nsec = new->tv_nsec - old->tv_nsec;
	d.tv_sec = new->tv_sec - old->tv_sec;
	return d;
}

static void timespec_add(struct timespec *t, struct timespec delta)
{
	t->tv_nsec += delta.tv_nsec;
	if (t->tv_nsec > ONE_BILLION) {
		t->tv_nsec %= ONE_BILLION;
		t->tv_sec += 1;
	}
	t->tv_sec += delta.tv_sec;
}

void ferite_profile_begin(FeriteScript *script)
{
	struct profile_entry *pe = hash_get_or_create(script->current_op_file, script->current_op_line);
	//fprintf(stderr, ">>> %s:%d\n", script->current_op_file, script->current_op_line);

	ferite_stack_push(NULL, pe->stack, create_timestamp());
	pe->ncalls++;
}

void ferite_profile_end(FeriteScript *script)
{
	struct profile_entry *pe = hash_get_or_create(script->current_op_file, script->current_op_line);
	struct timespec end, *start, duration;
	//fprintf(stderr, "<<< %s:%d\n", script->current_op_file, script->current_op_line);

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	start = ferite_stack_pop(NULL, pe->stack);
	duration = timespec_diff(start, &end);
	timespec_add(&pe->total_duration, duration);

	//fprintf(stderr, "Total duration for %s:%d = %ld.%ld\n", pe->filename, pe->line, pe->total_duration.tv_sec, pe->total_duration.tv_nsec);
	free(start);
}

void ferite_profile_save()
{
	int i;
	FILE *f;
	char *filename = "ferite.profile";

	f = fopen(filename, "w");
	if (f == NULL) {
		perror(filename);
		return;
	}

	for (i = 0; i < FERITE_PROFILE_NHASH; i++) {
		struct profile_entry *pe = profile_entries[i];
		if (pe == NULL)
			continue;
		do {
			fprintf(f, "%s:%d %d %ld.%ld\n", pe->filename, pe->line, pe->ncalls, pe->total_duration.tv_sec, pe->total_duration.tv_nsec);
			pe = pe->next;
		} while (pe);
	}

	if (fclose(f) == EOF)
		perror(filename);
}
