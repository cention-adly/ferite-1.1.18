#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif
#include <stdio.h>
#include "ferite.h"
#include <errno.h>

#define DIE(reason) do { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, reason); exit(1); } while(0)
#define ONE_BILLION 1000000000L

#define PROFILE_LINES 50
#define FERITE_PROFILE_NHASH 8192
#define FERITE_PROFILE_STACK_SIZE 5
int ferite_profile_enabled = FE_FALSE;
static char *profile_output = "ferite.profile";
static FeriteProfileEntry *profile_entries[FERITE_PROFILE_NHASH] = { NULL };

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
	if (r == 0)
		return 1;
	if (errno == ENOENT)
		return 0;

	fprintf(stderr, "Error stat'ing file %s\n", filename);
	perror(filename);
	return 0;
}

static int profile_line_entry_init_for_eval(FeriteProfileEntry *pe)
{
	pe->is_file = 0;
	pe->lines = fcalloc_ngc(sizeof(FeriteProfileLineEntry), PROFILE_LINES + 1);
	pe->line_count = PROFILE_LINES;

	return 0;
}
static int profile_line_entry_init_for_file(FeriteProfileEntry *pe)
{
	size_t line_count;

	if (get_line_count(pe->filename, &line_count) != 0) {
		fprintf(stderr, "Error getting line count for %s", pe->filename);
		return 1;
	}

	pe->is_file = 1;
	pe->lines = fcalloc_ngc(sizeof(FeriteProfileLineEntry),
			line_count
			+ 1 /* 1-based indexing, for unused lines[0] */
			+ 1 /* Count in EOF too */
			);
	pe->line_count = line_count + 1 /* EOF */;

	return 0;
}

static int profile_line_entry_init(FeriteProfileEntry *pe)
{
	if (file_exists(pe->filename))
		return profile_line_entry_init_for_file(pe);
	fprintf(stderr, "FIXME: file %s does not exist? (%s:%d)\n",
			pe->filename, __FILE__, __LINE__);
	return 0;
}

static FeriteProfileEntry *profile_init(char *filename)
{
	FeriteProfileEntry *pe;

	pe = fmalloc_ngc(sizeof(FeriteProfileEntry));
	pe->filename = ferite_strdup(filename, __FILE__, __LINE__);
	if (profile_line_entry_init(pe)) {
		return NULL;
	}

	pe->next = NULL;

	return pe;
}

static int is_profile_for(char *filename, FeriteProfileEntry *pe)
{
	return strcmp(pe->filename, filename) == 0;
}

static FeriteProfileEntry *find_in_linked_list(char *filename, FeriteProfileEntry *pe)
{
	while (pe) {
		if (is_profile_for(filename, pe))
			return pe;
		pe = pe->next;
	}
	return NULL;
}

static FeriteProfileEntry *hash_get(char *filename)
{
	unsigned int idx = hash(filename);

	return find_in_linked_list(filename, profile_entries[idx]);
}

static FeriteProfileEntry *hash_get_or_create(char *filename)
{
	FeriteProfileEntry *p = NULL;
	unsigned int idx = hash(filename);
	FeriteProfileEntry *pe = profile_entries[idx];

	if (pe) {
		p = find_in_linked_list(filename, pe);
		if (p)
			return p;
	}

	p = profile_init(filename);
	p->next = profile_entries[idx];
	profile_entries[idx] = p;

	return p;
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

void ferite_profile_begin(FeriteScript *script, size_t line)
{
	FeriteProfileEntry *pe;
	FeriteProfileLineEntry *le;
	// TODO lock based on script->lock

	pe = hash_get_or_create(script->current_op_file);
	if (pe == NULL) {
		fprintf(stderr, "Error creating profile entry for file %s\n", script->current_op_file);
		return;
	}
	le = &pe->lines[line];
	if (le->ncalls == 0)
		le->stack = ferite_create_stack(NULL, FERITE_PROFILE_STACK_SIZE);

	// TODO call create_timestamp() earlier
	ferite_stack_push(NULL, le->stack, create_timestamp());
	le->ncalls++;
}

void ferite_profile_end(FeriteScript *script, size_t line)
{
	FeriteProfileEntry *pe;
	FeriteProfileLineEntry *le;
	struct timespec end, *start, duration;

	// TODO lock based on script->lock

	pe = hash_get(script->current_op_file);
	if (pe == NULL) {
		fprintf(stderr, "FIXME No hash for file %s ???\n", script->current_op_file);
		return;
	}
	if (pe->line_count < line) {
		if (pe->is_file) {
			fprintf(stderr, "Error: Line number %lu exceeds the one we counted initially (%lu) for file %s",
				line, pe->line_count, script->current_op_file);
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
		fprintf(stderr, "Error got NULL start timespec for pop %s:%lu\n", pe->filename, line);
		fprintf(stderr, "for script->current_op_file [%s] script->current_op_line [%lu]\n", script->current_op_file, line);
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

void write_profile_line_entries(FILE *f, FeriteProfileEntry *pe) {
	FeriteProfileLineEntry *le;
	char path[PATH_MAX];
	size_t line_no;
	char *p = pe->filename;

	if (realpath(pe->filename, path) != NULL)
		p = path;
	else {
		if (ENOENT != errno) {
			perror(pe->filename);

		}
	}

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
		FeriteProfileEntry *pe = profile_entries[i];
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
