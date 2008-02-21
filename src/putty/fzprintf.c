#include "putty.h"
#include "misc.h"

int fznotify(sftpEventTypes type)
{
    fprintf(stdout, "%c", (int)type + '0');
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
		fprintf(stdout, "%c%s\n", (int)type + '0', s);
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
		fprintf(stdout, "%c%s\n", (int)type + '0', s);
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

int fzprintf_raw_untrusted(sftpEventTypes type, const char* fmt, ...)
{
    va_list ap;
    char* str, *p, *s;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);
    p = str;
    s = str;
    while (*p)
    {
	if (*p == '\r')
	    p++;
	else if (*p == '\n')
	{
	    if (s != str)
		*s++ = ' ';
	    p++;
	}
	else if (*p)
	    *s++ = *p++;
    }
    while (s != str && *(s - 1) == ' ')
	s--;
    *s = 0;
    if (*str)
	fprintf(stdout, "%c%s\n", (int)type + '0', str);

    sfree(str);

    va_end(ap);

    fflush(stdout);

    return 0;
}

int fzprintf_raw(sftpEventTypes type, const char* fmt, ...)
{
    va_list ap;
    char* str ;
    va_start(ap, fmt);
    str = dupvprintf(fmt, ap);

    fprintf(stdout, "%c", (int)type + '0');
    fprintf(stdout, str);

    sfree(str);

    va_end(ap);

    fflush(stdout);

    return 0;
}

int fznotify1(sftpEventTypes type, int data)
{
    fprintf(stdout, "%c%d\n", (int)type + '0', data);
    fflush(stdout);
    return 0;
}

