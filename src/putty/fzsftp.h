int ProcessQuotaCmd(const char* line);
int RequestQuota(int i, int bytes);
void UpdateQuota(int i, int bytes);
char* get_input_pushback(void);
int has_input_pushback(void);
#ifndef _WINDOWS
char* read_input_line(int force, int* error);
#endif
