#ifndef __FZSFTP_H__
#define __FZSFTP_H__

int ProcessQuotaCmd(const char* line);
int RequestQuota(int i, int bytes);
void UpdateQuota(int i, int bytes);
char* get_input_pushback(void);
int has_input_pushback(void);
#ifndef _WINDOWS
char* read_input_line(int force, int* error);
#endif

#ifdef _WINDOWS
#include <Windows.h>
typedef FILETIME _fztimer;
#else
typedef struct
{
	unsigned int low;
    time_t high;
} _fztimer;
#endif

void fz_timer_init(_fztimer *timer);
int fz_timer_check(_fztimer *timer);

#endif //__FZSFTP_H__
