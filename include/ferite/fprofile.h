#ifndef __FERITE_PROFILE_H__
#define __FERITE_PROFILE_H__

#include "time.h"

extern int ferite_profile_enabled;
#define FERITE_PROFILE_BEGIN(script, line) \
	do { if (ferite_profile_enabled) \
		ferite_profile_begin(script, line); \
	} while(0)
#define FERITE_PROFILE_END(script, line) \
	do { if (ferite_profile_enabled) \
		ferite_profile_end(script, line); \
	} while(0)
#define FERITE_PROFILE_SAVE() \
	do { if (ferite_profile_enabled) \
		ferite_profile_save(); \
	} while(0)

void ferite_profile_begin(FeriteScript *script, size_t line);
void ferite_profile_end(FeriteScript *script, size_t line);
void ferite_profile_save();
// FIXME rename this to ferite_profile_set_filename_pattern
void ferite_profile_output(char *filename);
#endif
