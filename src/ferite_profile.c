#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif
#include <stdio.h>
#include "ferite.h"
#include <errno.h>

#define DIE(reason) do { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, reason); exit(1); } while(0)
#define ONE_BILLION 1000000000L

int ferite_profile_enabled = FE_FALSE;

#define PROFILE_LINES 50
#define FERITE_PROFILE_NHASH 8192
#define FERITE_PROFILE_STACK_SIZE 5
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

static int get_line_count(char *filename, size_t *count)
{
	FILE *f = fopen(filename, "r");
	int ch;
	int lines = 0;

	if (f == NULL) {
		perror(filename);
		return 1;
	}

	while (EOF != (ch=fgetc(f)))
		if (ch=='\n')
			lines++;
	*count = lines;

	if (fclose(f) == EOF) {
		perror(filename);
	}
	return 0;
}

static int file_exists(const char *filename)
{
	struct stat st;
	int r = stat(filename, &st);
	if (r == ENOENT)
		return 0;
	return 1;
}

static int profile_line_entry_init_for_eval(struct profile_entry *pe)
{
	pe->is_file = 0;
	pe->lines = fcalloc_ngc(sizeof(struct profile_line_entry), PROFILE_LINES + 1);
	pe->line_count = PROFILE_LINES;

	return 0;
}
static int profile_line_entry_init_for_file(struct profile_entry *pe)
{
	size_t line_count;

	if (get_line_count(pe->filename, &line_count) != 0) {
		fprintf(stderr, "Error getting line count for %s", pe->filename);
		return 1;
	}

	pe->is_file = 1;
	pe->lines = fcalloc_ngc(sizeof(struct profile_line_entry), line_count + 1);
	pe->line_count = line_count;

	return 0;
}

static int profile_line_entry_init(struct profile_entry *pe)
{
	if (file_exists(pe->filename))
		return profile_line_entry_init_for_file(pe);

	return profile_line_entry_init_for_eval(pe);
}

static struct profile_entry *profile_init(char *filename)
{
	struct profile_entry *pe;

	pe = fmalloc_ngc(sizeof(struct profile_entry));
	pe->filename = ferite_strdup(filename, __FILE__, __LINE__);
	if (profile_line_entry_init(pe)) {
		return NULL;
	}

	pe->next = NULL;

	return pe;
}

static int is_profile_for(char *filename, struct profile_entry *pe)
{
	return strcmp(pe->filename, filename) == 0;
}

static struct profile_entry *hash_get(char *filename)
{
	unsigned int idx = hash(filename);

	return profile_entries[idx];
}

static struct profile_entry *hash_get_or_create(char *filename)
{
	struct profile_entry *p = NULL;
	unsigned int idx = hash(filename);
	struct profile_entry *pe = profile_entries[idx], *tail;

	if (pe == NULL) {
		profile_entries[idx] = profile_init(filename);
		return profile_entries[idx];
	} else {
		p = pe;
		while (p) {
			if (is_profile_for(filename, pe))
				return pe;
			tail = p;
			p = p->next;
		}
	}

	tail->next = profile_init(filename);

	return tail;
}

int ferite_profile_toggle(int state)
{
	return ferite_profile_enabled = state;
}

static struct timespec *create_timestamp()
{
	struct timespec *t = fmalloc_ngc(sizeof(struct timespec));

	if (clock_gettime(CLOCK_MONOTONIC_RAW, t))
		perror("create_timestamp()");
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

void ferite_profile_begin(char *filename, size_t line, unsigned int depth)
{
	struct profile_entry *pe;
	struct profile_line_entry *le;

	pe = hash_get_or_create(filename);
	if (pe == NULL) {
		fprintf(stderr, "Error creating profile entry for file %s\n", filename);
		return;
	}
	le = &pe->lines[line];
	if (le->ncalls == 0)
		le->stack = ferite_create_stack(NULL, FERITE_PROFILE_STACK_SIZE);

	// TODO call create_timestamp() earlier
	ferite_stack_push(NULL, le->stack, create_timestamp());
	le->ncalls++;
}

void ferite_profile_end(char *filename, size_t line, unsigned int depth)
{
	struct profile_entry *pe;
	struct profile_line_entry *le;
	struct timespec end, *start, duration;

	pe = hash_get(filename);
	if (pe == NULL) {
		fprintf(stderr, "No hash for file %s ???", filename);
		return;
	}
	if (pe->line_count < line) {
		if (pe->is_file) {
			fprintf(stderr, "Error: Line number %lu exceeds the one we counted initially (%lu) for file %s",
				line, pe->line_count, filename);
			exit(1);
		} else {
			size_t add = line - pe->line_count;
			if (add < PROFILE_LINES) {
				add = PROFILE_LINES;
			}

			pe->lines = frealloc_ngc(pe->lines, pe->line_count + add);
			if (pe->lines == NULL) {
				fprintf(stderr, "OOM reallocating profile lines record\n");
				exit(1);
			}
			bzero(pe->lines + pe->line_count + 1, add);
			pe->line_count += add;
		}
	}

	le = &pe->lines[line];

	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	start = ferite_stack_pop(NULL, le->stack);
	if (start == NULL) {
		fprintf(stderr, "Error got NULL start timespec for pop %s:%d\n", pe->filename, line);
		fprintf(stderr, "for script->current_op_file [%s] script->current_op_line [%d]\n", filename, line);
		return;

	}
	duration = timespec_diff(start, &end);
	timespec_add(&le->total_duration, duration);

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

void write_profile_line_entries(FILE *f, struct profile_entry *pe) {
	struct profile_line_entry *le;
	char path[PATH_MAX];
	size_t line_no;
	char *p = pe->filename;

	if (realpath(pe->filename, path) != NULL)
		p = path;
	else
		perror("hahah");

	for (line_no = 1; line_no <= pe->line_count; line_no++) {
		le = &pe->lines[line_no];
		if (le->ncalls > 0) {
			fprintf(f, "%7d %4ld.%-9ld %s:%lu\n",
				le->ncalls,
				le->total_duration.tv_sec,
				le->total_duration.tv_nsec,
				p,
				line_no
				);
			// if (le->stack->stack_ptr) {
			// 	fprintf(stderr, "Stack size of %s:%lu is %d???\n", p, line_no, le->stack->stack_ptr);
			// }
		}
	}
}

void ferite_profile_save()
{
	int i;
	FILE *f;
	char filename[PATH_MAX];

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
		while (pe) {
			write_profile_line_entries(f, pe);
			pe = pe->next;
		}
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
