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

int bytesAvailable[2] = { 0, 0 };

char* input_pushback = 0;

#ifndef _WINDOWS
#include <unistd.h>

char *input_buf = 0;
int input_buflen = 0, input_bufsize = 0;
#endif

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
	DWORD read;
	BOOL r;
	char buffer[21];

	r = ReadFile(hin, buffer, 20, &read, 0);
	if (!r || read == 0)
	    fatalbox("ReadFile failed in ReadQuotas");
	buffer[read] = 0;

	if (buffer[0] != '-')
	{
	    if (input_pushback != 0)
		fatalbox("input_pushback not null!");
	    else
		input_pushback = strdup(buffer);
	}
	else
	    ProcessQuotaCmd(buffer);
    }

    SetConsoleMode(hin, savemode);
#else
    char* line;
    while (bytesAvailable[i] == 0)
    {
	int error = 0;
	line = read_input_line(1, &error);
	if (line == NULL || error)
	{
	    fatalbox("read_input_line failed in ReadQuotas");
	    break;
	}

	if (line[0] != '-')
	{
	    if (input_pushback != 0)
		fatalbox("input_pushback not null!");
	    else
		input_pushback = strdup(line);
	}
	else
	    ProcessQuotaCmd(line);
	sfree(line);
    }
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

int ProcessQuotaCmd(const char* line)
{
	int direction = 0, number, pos;

    if (line[0] != '-')
	return 0;

    if (line[1] == '0')
	direction = 0;
    else if (line[1] == '1')
	direction = 1;
    else
	fatalbox("Invalid data received in ReadQuotas: Unknown direction");

    if (line[2] == '-')
    {
	bytesAvailable[direction] = -1;
	return 0;
    }

    number = 0;
    for (pos = 2;; pos++)
    {
	if (line[pos] == 0 || line[pos] == '\r' || line[pos] == '\n')
	    break;
	if (line[pos] < '0' || line[pos] > '9')
	    fatalbox("Invalid data received in ReadQuotas: Bytecount not a number");

	number *= 10;
	number += line[pos] - '0';
    }
    if (bytesAvailable[direction] == -1)
	bytesAvailable[direction] = number;
    else
	bytesAvailable[direction] += number;

    return 1;
}

char* get_input_pushback()
{
    char* pushback = input_pushback;
    input_pushback = 0;
    return pushback;
}

int has_input_pushback()
{
    if (input_pushback != 0)
	return 1;
    else
	return 0;
}

#ifndef _WINDOWS
static void clear_input_buffers(int free)
{
    if (free && input_buf != NULL)
	sfree(input_buf);
    input_buf = 0;
    input_bufsize = 0;
    input_buflen = 0;
}

char* read_input_line(int force, int* error)
{
    int ret;
    do {
	if (input_buflen >= input_bufsize) {
	    input_bufsize = input_buflen + 512;
	    input_buf = sresize(input_buf, input_bufsize, char);
	}
	ret = read(0, input_buf+input_buflen, 1);
	if (ret < 0) {
	    perror("read");
	    *error = 1;
	    clear_input_buffers(1);
	    return NULL;
	}
	if (ret == 0) {
	    /* eof on stdin; no error, but no answer either */
	    *error = 1;
	    clear_input_buffers(1);
	    return NULL;
	}

	if (input_buf[input_buflen++] == '\n') {
	    /* we have a full line */
	    char* buf = input_buf;
	    clear_input_buffers(0);
	    return buf;
	}
    } while(force);

    return NULL;
}
#endif
