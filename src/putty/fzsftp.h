typedef enum
{
    sftpUnknown = -1,
    sftpReply = 0,
    sftpDone,
    sftpError,
    sftpVerbose,
    sftpStatus,
    sftpRecv,
    sftpSend,
    sftpClose,
    sftpRequest,
    sftpListentry,
    sftpRead,
    sftpWrite,
    sftpRequestPreamble,
    sftpRequestInstruction,
    sftpUsedQuotaRecv,
    sftpUsedQuotaSend
} sftpEventTypes;

enum sftpRequestTypes
{
    sftpReqPassword,
    sftpReqHostkey,
    sftpReqHostkeyChanged,
    sftpReqUnknown
};

int fznotify(sftpEventTypes type);
int fzprintf(sftpEventTypes type, const char* p, ...);
int fzprintf_raw(sftpEventTypes type, const char* p, ...);
int fzprintf_raw_untrusted(sftpEventTypes type, const char* p, ...);
int fznotify1(sftpEventTypes type, int data);

int ProcessQuotaCmd(const char* line);
int RequestQuota(int i, int bytes);
void UpdateQuota(int i, int bytes);
char* get_input_pushback(void);
int has_input_pushback(void);
#ifndef _WINDOWS
char* read_input_line(int force, int* error);
#endif
