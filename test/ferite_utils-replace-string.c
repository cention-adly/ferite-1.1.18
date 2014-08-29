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

int main(int argc, char *argv[])
{
	size_t i;
	size_t ntests = sizeof(tests) / sizeof(tests[0]);

	ferite_init(argc, argv);

	for (i = 0; i < ntests; i++) {
		struct TestVector *t = &tests[i];

		char *got = ferite_replace_string(t->text, t->replace, t->with);
		is_str(got, t->want, "Replace '%s' in '%s' with '%s' must become '%s'",
				t->replace, t->text, t->with, t->want);
	}

	return done_testing();
}
