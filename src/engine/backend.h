#ifndef __BACKEND_H__
#define __BACKEND_H__

class CBackend
{
public:
	virtual void Read(void *data, unsigned int len) = 0;
	virtual void Write(const void *data, unsigned int len) = 0;
	virtual bool Error() const = 0;
	virtual int LastError() const = 0;
	virtual unsigned int LastCount() const = 0;
};

class CSocketBackend : public CBackend
{
public:
	CSocketBackend(wxSocketBase* pSocket) : m_pSocket(pSocket) {}
	// Backend definitions
	virtual void Read(void *buffer, unsigned int len) { m_pSocket->Read(buffer, len); }
	virtual void Write(const void *buffer, unsigned int len) { m_pSocket->Write(buffer, len); }
	virtual bool Error() const { return m_pSocket->Error(); }
	virtual unsigned int LastCount() const { return m_pSocket->LastCount(); }
	virtual int LastError() const { return m_pSocket->LastError(); }

protected:
	wxSocketBase* m_pSocket;
};

#endif //__BACKEND_H__