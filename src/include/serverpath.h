#ifndef __SERVERPATH_H__
#define __SERVERPATH_H__

class CServerPath
{
public:
	CServerPath();
	CServerPath(const wxString &path);
	CServerPath(ServerProtocol protocol, const wxString &path);
	virtual ~CServerPath();
};

#endif
