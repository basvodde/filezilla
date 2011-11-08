#include <filezilla.h>
#include "timeformatting.h"
#include "Options.h"
#include <option_change_event_handler.h>

namespace {

class Impl : public COptionChangeEventHandler
{
public:
	Impl()
	{
		InitFormat();

		RegisterOption(OPTION_DATE_FORMAT);
		RegisterOption(OPTION_TIME_FORMAT);
	}

	void InitFormat()
	{
		const wxString& dateFormat = COptions::Get()->GetOption(OPTION_DATE_FORMAT);
		const wxString& timeFormat = COptions::Get()->GetOption(OPTION_TIME_FORMAT);

		if (dateFormat == _T("1"))
			m_dateFormat = _T("%Y-%m-%d");
		else if (dateFormat[0] == '2')
			m_dateFormat = dateFormat.Mid(1);
		else
			m_dateFormat = _T("%x");

		m_dateTimeFormat = m_dateFormat;
		m_dateTimeFormat += ' ';

		if (timeFormat == _T("1"))
			m_dateTimeFormat += _T("%H:%M");
		else if (timeFormat[0] == '2')
			m_dateTimeFormat += timeFormat.Mid(1);
		else
			m_dateTimeFormat += _T("%X");
	}

	virtual void OnOptionChanged(int /*option*/)
	{
		InitFormat();
	}

	wxString m_dateFormat;
	wxString m_dateTimeFormat;
};

Impl& GetImpl()
{
	static Impl impl;
	return impl;
}
}

wxString CTimeFormat::FormatDateTime(const wxDateTime &time)
{
	Impl& impl = GetImpl();

	return time.Format(impl.m_dateTimeFormat);
}

wxString CTimeFormat::FormatDate(const wxDateTime &time)
{
	Impl& impl = GetImpl();

	return time.Format(impl.m_dateFormat);
}