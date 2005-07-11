typedef enum
{
    sftpReply = 0,
    sftpDone,
    sftpError,
    sftpVerbose,
    sftpStatus,
    sftpRecv,
    sftpSend,
    sftpClose,
    sftpRequest,
    sftpUnknown
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
int fznotify1(sftpEventTypes type, int data);
