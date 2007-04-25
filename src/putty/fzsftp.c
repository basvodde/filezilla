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

static int bytesAvailable[2] = { 0, 0 };

static int ReadQuotas(int i)
{
#ifdef _WINDOWS
    HANDLE hin;
    DWORD savemode, newmode;

    hin = GetStdHandle(STD_INPUT_HANDLE);

    GetConsoleMode(hin, &savemode);
    newmode = savemode | ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT;
    newmode &= ~ENABLE_ECHO_INPUT;
    SetConsoleMode(hin, newmode);

    while (bytesAvailable[i] == 0)
    {
	int direction, number;
	DWORD read, pos;
	BOOL r;
	char buffer[20];

	r = ReadFile(hin, buffer, 20, &read, 0);
	if (!r)
	    fatalbox("ReadFile failed in ReadQuotas");

	if (read < 3)
	    continue;

	if (buffer[0] != '-')
	    continue;

	if (buffer[1] == '0')
	    direction = 0;
	else if (buffer[1] == '1')
	    direction = 1;
	else
	    fatalbox("Invalid data received in ReadQuotas: Unknown direction");

	if (buffer[2] == '-')
	{
	    bytesAvailable[direction] = -1;
	    continue;
	}
	
	number = 0;
	for (pos = 2; pos < read; pos++)
	{
	    if (buffer[pos] == '\r' || buffer[pos] == '\n')
		break;
	    if (buffer[pos] < '0' || buffer[pos] > '9')
		fatalbox("Invalid data received in ReadQuotas: Bytecount not a number");

	    number *= 10;
	    number += buffer[pos] - '0';
	}
	if (bytesAvailable[direction] == -1)
	    bytesAvailable[direction] = number;
	else
	    bytesAvailable[direction] += number;
    }

    SetConsoleMode(hin, savemode);
#else
    bytesAvailable[0] = -1;
    bytesAvailable[1] = -1;
#endif //_WINDOWS
    return 1;
}

int RequestQuota(int i, int bytes)
{
    if (bytesAvailable[i] == 0)
    {
	bytesAvailable[i] = 0;
	fznotify(sftpUsedQuotaRecv + i);
    }
    if (bytesAvailable[i] == 0)
    {
	ReadQuotas(i);
    }

    if (bytesAvailable[i] == -1)
	return bytes;

    if (bytesAvailable[i] > bytes)
	return bytes;

    return bytesAvailable[i];
}

void UpdateQuota(int i, int bytes)
{
    if (bytesAvailable[i] == -1)
	return;

    if (bytesAvailable[i] > bytes)
	bytesAvailable[i] -= bytes;
    else
	bytesAvailable[i] = 0;
}
