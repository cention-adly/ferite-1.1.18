#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__ 1
void ferite_trace_function_entry(int level, char *file, unsigned int line, char *name);
void ferite_trace_function_exit(int level, char *file, unsigned int line, char *name);
void ferite_trace_record();
#endif
