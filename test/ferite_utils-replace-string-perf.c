#include "tap.h"
#include "ferite.h"
#include "test-time.h"

struct TestVector {
	char *text;
	char *replace;
	char *with;
	char *want;
};

#define MALLOC(x) (ferite_malloc)((x), __FILE__, __LINE__, NULL)

struct TestVector* create_perf_test_vector(size_t nchar)
{
	size_t i;
	struct TestVector *ret = MALLOC(sizeof(struct TestVector));
	ret->text = MALLOC(nchar+1);
	ret->want = MALLOC(nchar+1);

	for (i = 0; i < nchar; i++) {
		ret->text[i] = (i % 2 > 0 ? 'a':'B');
	}
	memset(ret->want, 'B', nchar);
	ret->text[i] = ret->want[i] = '\0';
	ret->replace = "a" ;
	ret->with = "B";

	return ret;
}

char *old_ferite_replace_string( char *str, char *pattern, char *data )
{
    size_t i = 0, start = 0;
    char *rstr = NULL, *tmpbuf = NULL;

    FE_ENTER_FUNCTION;
    if( str && pattern && data )
    {
		/* empty string -- nothing to replace */
        if( !str[0] )
        {
            FE_LEAVE_FUNCTION(  fstrdup(str) );
        }
		/* empty pattern -- nothing to replace */
        if( !pattern[0] )
        {
            FE_LEAVE_FUNCTION(  fstrdup(str) );
        }

        if( !data[0] ) /* empty replacement -- string won't grow */
			rstr = fcalloc_ngc( strlen( str ) + 1, sizeof(char) );
        else /* none of the strings can have length zero now */
			rstr = fcalloc_ngc( strlen( str ) * strlen( pattern ) * strlen( data ) + 1, sizeof(char) );

        FUD(("replace_str: replace \"%s\" with \"%s\"\n", pattern, data ));
        while( ((i=ferite_find_string( str+start, pattern ))+1) )
        {
            strncat( rstr, str+start, i );
            strcat( rstr, data );
            start = i + start + strlen(pattern);
        }
        strcat( rstr, str + start );
        tmpbuf = fstrdup( rstr );
        ffree_ngc( rstr );
        FE_LEAVE_FUNCTION( tmpbuf );
    }
    FE_LEAVE_FUNCTION( NULL );
}

void testStringReplace(void) {
	char *old_got, *new_got;
	struct TestVector *t;
	unsigned long start, old_duration, new_duration;
	double ratio;
	const size_t NCHARS = 10000;

	t = create_perf_test_vector(NCHARS);

	// Run both to warmup the caches
	old_got = old_ferite_replace_string(t->text, t->replace, t->with);
	new_got = ferite_replace_string(t->text, t->replace, t->with);

	// Now we time each old and new
	start = get_time_nanos();
	old_got = old_ferite_replace_string(t->text, t->replace, t->with);
	old_duration = get_time_nanos() - start;

	start = get_time_nanos();
	new_got = ferite_replace_string(t->text, t->replace, t->with);
	new_duration = get_time_nanos() - start;

	is_str(old_got, t->want, "old 'got' matches 'want'");
	is_str(new_got, t->want, "new 'got' matches 'want'");
	ok (new_duration - old_duration > 0, "new is faster");
	diag("old_duration - new_duration = %ld", old_duration - new_duration);
	ratio = (double) old_duration / new_duration;
	ok (ratio > 1, "new is faster: %f", ratio);
}

int main(int argc, char *argv[])
{
	ferite_init(argc, argv);
	testStringReplace();

	return done_testing();
}
