#ifdef HAVE_CONFIG_HEADER
#include "../config.h"
#endif

#include "ferite.h"
#include "fprofile.h"

#include <stdio.h>
static unsigned int function_entry_counter = 0;
static unsigned int function_exit_counter = 0;
static unsigned int save_to_file_threshold = 100;

static struct iron_profile_Trie profile = {
   .isTerminal = 0,
   .letter = '\0',
   .children = { NULL },
   .enter = NULL,
   .exit = NULL,
};

static void save_trace_data() {
   iron_trace_record();
}

void iron_trace_function_entry(int level, char *file, unsigned int line, char *name)
{
   function_entry_counter++;
}

void iron_trace_function_exit(int level, char *file, unsigned int line, char *name)
{
   function_exit_counter++;
   if (function_exit_counter % save_to_file_threshold == 0) {
      save_trace_data();
   }
}

void iron_trace_record()
{
   fprintf(stderr, "Total function_entry_counter: %d, exit: %d\n", function_entry_counter, function_exit_counter);
}
