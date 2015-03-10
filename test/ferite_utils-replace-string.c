#include "tap.h"
#include "ferite.h"

struct TestVector {
	char *text;
	char *replace;
	char *with;
	char *want;
};

struct TestVector tests[] = {
	// Replace nothing
	{
		.text = "Hello world",
		.replace = "",
		.with = "",
		.want = "Hello world",
	},
	// Shorten word
	{
		.text = "Hello delete world",
		.replace = "delete",
		.with = "bear",
		.want = "Hello bear world",
	},
	{
		.text = "delete Hello world",
		.replace = "delete ",
		.with = "bear ",
		.want = "bear Hello world",
	},
	{
		.text = "Hello world delete",
		.replace = " delete",
		.with = " bear",
		.want = "Hello world bear",
	},
	// Expand word
	{
		.text = "aba",
		.replace = "b",
		.with = "BBB",
		.want = "aBBBa",
	},
	{
		.text = "baa",
		.replace = "b",
		.with = "BBB",
		.want = "BBBaa",
	},
	{
		.text = "aab",
		.replace = "b",
		.with = "BBB",
		.want = "aaBBB",
	},
	// Delete word
	{
		.text = "Hello delete world",
		.replace = "delete ",
		.with = "",
		.want = "Hello world",
	},
	{
		.text = "delete Hello world",
		.replace = "delete ",
		.with = "",
		.want = "Hello world",
	},
	{
		.text = "Hello world delete",
		.replace = " delete",
		.with = "",
		.want = "Hello world",
	},
	// Delete single-letter word
	{
		.text = "Hello Xworld",
		.replace = "X",
		.with = "",
		.want = "Hello world",
	},
	{
		.text = "XHello world",
		.replace = "X",
		.with = "",
		.want = "Hello world",
	},
	{
		.text = "Hello worldX",
		.replace = "X",
		.with = "",
		.want = "Hello world",
	},
	// Expand single-letter word
	{
		.text = "Hello Xworld",
		.replace = "X",
		.with = "big ",
		.want = "Hello big world",
	},
	{
		.text = "XHello world",
		.replace = "X",
		.with = "Big ",
		.want = "Big Hello world",
	},
	{
		.text = "Hello world bye",
		.replace = "X",
		.with = " bye",
		.want = "Hello world bye",
	},
};

void test_replace() {
	size_t i;
	size_t ntests = sizeof(tests) / sizeof(tests[0]);

	for (i = 0; i < ntests; i++) {
		struct TestVector *t = &tests[i];

		char *got = ferite_replace_string(t->text, t->replace, t->with);
		is_str(got, t->want, "Replace '%s' in '%s' with '%s' must become '%s'",
				t->replace, t->text, t->with, t->want);
	}
}

#define NCHARS (1024*1024)
#define N (NCHARS+1)
void test_replace_huge_string1() {
	size_t i;

	char txt[N];
	for (i = 0; i < NCHARS; i++) {
		txt[i] = i % 2 ? 'a':'A';
	}
	txt[i] = '\0';

	char *got = ferite_replace_string(txt, "A", "B");
	int good = 1;
	for (i = 0; i < NCHARS; i++) {
		char want = i % 2 ? 'a':'B';
		if (got[i] != want) {
			good = 0;
		}
	}

	ok(good, "replace_huge_string1");
}

#define NCHARS (1024*1024)
#define N (NCHARS+1)
void test_replace_huge_string2() {
	size_t i;

	char txt[N];
	for (i = 0; i < NCHARS; i++) {
		txt[i] = i % 2 ? 'a':'A';
	}
	txt[i] = '\0';

	char *got = ferite_replace_string(txt, "A", "BB");
	//printf("txt %d[%s]\n",strlen(txt), txt);
	//printf("got %d[%s]\n",strlen(got),  got);
	int gotLen = strlen(got);
	is (gotLen, NCHARS + NCHARS / 2, "Length of new text must be 1.5 times the previous one");
	int good = 1;
	for (i = 0; i < NCHARS; i++) {
		if (i%2) {
			// 'a' must not be replaced
			int j = i + i / 2 + 1;
			if (got[j] != 'a') {
				diag("FAIL: got[%d] != 'a'!", j);
				good = 0;
			}
		} else {
			int j = i + i / 2;
			if (strncmp("BB", got + j, 2) != 0) {
				diag("FAIL: got[%d:%d] must be BB", j, j+2);
				good = 0;
			}
		}
	}

	ok(good, "ferite_replace_string2");
}

int main(int argc, char *argv[])
{
	ferite_init(argc, argv);

	test_replace();
	test_replace_huge_string1();
	test_replace_huge_string2();

	return done_testing();
}
