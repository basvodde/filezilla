#include "putty.h"
#include "misc.h"

int fznotify(sftpEventTypes type)
{
    fprintf(stdout, "%d", (int)type);
    fflush(stdout);
    return 0;
}

int fzprintf(sftpEventTypes type, const char* fmt, ...)
{
    va_list ap;
    char* str, *p, *s;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    p = str;
    s = str;
    while (1)
    {
	if (*p == '\r' || *p == '\n')
	{
	    if (p != s)
	    {
		*p = 0;
		fprintf(stdout, "%d%s\n", (int)type, s);
		s = p + 1;
	    }
	    else
		s++;
	}
	else if (!*p)
	{
	    if (p != s)
	    {
		*p = 0;
		fprintf(stdout, "%d%s\n", (int)type, s);
		s = p + 1;
	    }
	    break;
	}
	p++;
    }
    sfree(str);

    va_end(ap);

    fflush(stdout);

    return 0;
}

int fzprintf_raw(sftpEventTypes type, const char* fmt, ...)
{
    va_list ap;
    char* str, *p, *s;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);

    fprintf(stdout, "%d", (int)type);
    fprintf(stdout, str);

    sfree(str);

    va_end(ap);

    fflush(stdout);

    return 0;
}

int fznotify1(sftpEventTypes type, int data)
{
    fprintf(stdout, "%d%d\n", (int)type, data);
    fflush(stdout);
    return 0;
}
