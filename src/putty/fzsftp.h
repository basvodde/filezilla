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
    sftpUnknown
} sftpEventTypes;

enum sftpCommands
{
    sftpConnect // Parameters: strhost, port, struser, strpass 
};

int fznotify(sftpEventTypes type);
int fzprintf(sftpEventTypes type, const char* p, ...);
int fznotify1(sftpEventTypes type, int data);
