#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif
#include <stdio.h>
#include "ferite.h"

#define DIE(reason) do { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, reason); exit(1); } while(0)
#define ONE_BILLION 1000000000L

int ferite_profile_enabled = FE_FALSE;

#define FERITE_PROFILE_NHASH 8192
#define FERITE_PROFILE_STACK_SIZE 20
static struct profile_entry *profile_entries[FERITE_PROFILE_NHASH] = { NULL };
static char *profile_output = "ferite.profile";

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

static int number_width(unsigned int num)
{
	unsigned int width = 0;

	do {
		width += 1;
		num /= 10;
	} while (num > 0);

	return width;
}

static char *hash_get_key(char *filename, unsigned int lnum)
{
	size_t len = strlen(filename);
	char *key;
	int ntruncated = 0;

	len += number_width(lnum);
	len += 2; // filename:lnum\0
		  //         ^     ^
	key = fmalloc_ngc(len);
	if ((ntruncated = snprintf(key, len, "%s:%d", filename, lnum)) >= len) {
		fprintf(stderr, "FIXME: %d bytes for key '%s' truncated for %s:%d\n", ntruncated, key, filename, lnum);
	}
	return key;
}

static struct profile_entry *ferite_profile_init(char *filename, int line)
{
	struct profile_entry *pe;

	pe = fmalloc_ngc(sizeof(struct profile_entry));
	pe->filename = ferite_strdup(filename, __FILE__, __LINE__);
	pe->ncalls = 0;
	pe->total_duration.tv_sec = 0;
	pe->total_duration.tv_nsec = 0;
	pe->line = line;
	pe->stack = ferite_create_stack(NULL, FERITE_PROFILE_STACK_SIZE);

	pe->next = NULL;

	return pe;
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

int ferite_profile_toggle(int state)
{
	return ferite_profile_enabled = state;
}

static struct timespec *create_timestamp()
{
	struct timespec *t = fmalloc_ngc(sizeof(struct timespec));

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

void ferite_profile_begin(char *filename, unsigned int line, unsigned int depth)
{
	struct profile_entry *pe;

	pe = hash_get_or_create(filename, line);
	//fprintf(stderr, ">>> %s:%d\n", script->current_op_file, script->current_op_line);

	// fprintf(stderr, "push depth %d %s:%d\n", depth, pe->filename, pe->line);
	ferite_stack_push(NULL, pe->stack, create_timestamp());
	pe->ncalls++;
}

void ferite_profile_end(char *filename, unsigned int line, unsigned int depth)
{
	struct profile_entry *pe;
	struct timespec end, *start, duration;

	pe = hash_get_or_create(filename, line);
	//fprintf(stderr, "<<< %s:%d\n", script->current_op_file, script->current_op_line);

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	//fprintf(stderr, "pop depth %d %s:%d\n", depth, pe->filename, pe->line);
	start = ferite_stack_pop(NULL, pe->stack);
	if (start == NULL) {
		// fprintf(stderr, "Error got NULL start timespec for pop %s:%d\n", pe->filename, pe->line);
		// fprintf(stderr, "for script->current_op_file [%s] script->current_op_line [%d]\n", filename, line);
		return;

	}
	duration = timespec_diff(start, &end);
	timespec_add(&pe->total_duration, duration);

	//fprintf(stderr, "Total duration for %s:%d = %ld.%ld\n", pe->filename, pe->line, pe->total_duration.tv_sec, pe->total_duration.tv_nsec);
	ffree_ngc(start);
}

static int format_profile_filename(char *format, char *buf)
{
	struct tm now;
	time_t t;

	(void)time(&t);
	(void)localtime_r(&t, &now);

	return strftime(buf, PATH_MAX, format, &now) != 0;
}

void ferite_profile_save()
{
	int i;
	FILE *f;
	char path[PATH_MAX];
	char filename[PATH_MAX];
	char *p;

	if (!format_profile_filename(profile_output, filename)) {
		fprintf(stderr, "Error: profile output '%s' results in empty filename\n", profile_output);

		return;
	}

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

			if (realpath(pe->filename, path) == NULL) {
				perror(pe->filename);
				p = pe->filename;
			} else {
				p = path;
			}
			fprintf(f, "%7d %4ld.%-9ld %s:%d\n",
				pe->ncalls,
				pe->total_duration.tv_sec,
				pe->total_duration.tv_nsec,
				p,
				pe->line
				);
			if (pe->stack->stack_ptr) {
				fprintf(stderr, "Stack size of %s:%d is %d???\n", pe->filename, pe->line, pe->stack->stack_ptr);
			}
			pe = pe->next;
		} while (pe);
	}

	if (fclose(f) == EOF)
		perror(filename);
}

void ferite_profile_output(char *filename) {
	int pid = getpid();
	int len = strlen(filename) + number_width(pid) + 2;

	profile_output = malloc(len);
	if (profile_output == NULL) {
		DIE("OOM");
	}
	snprintf(profile_output, len, "%s.%d", filename, pid);
}
