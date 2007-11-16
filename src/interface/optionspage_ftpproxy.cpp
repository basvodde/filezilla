#include "FileZilla.h"

#include "Options.h"
#include "settingsdialog.h"
#include "optionspage.h"
#include "optionspage_ftpproxy.h"

bool COptionsPageFtpProxy::LoadPage()
{
	bool failure = false;

	return !failure;
}

bool COptionsPageFtpProxy::SavePage()
{
	return true;
}

bool COptionsPageFtpProxy::Validate()
{
	return true;
}
