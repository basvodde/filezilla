#ifndef __FTPCONTROLSOCKET_H__
#define __FTPCONTROLSOCKET_H__

#pragma once
#include "controlsocket.h"

class CFtpControlSocket : public CControlSocket
{
public:
	CFtpControlSocket();
	virtual ~CFtpControlSocket();
};

#endif