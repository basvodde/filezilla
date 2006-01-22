#include "FileZilla.h"
#include "wrapengine.h"
#include <wx/wizard.h>
#include "filezillaapp.h"

// Chinese equivalents to ".", "," and ":"
static const wxChar wrapAfter_Chinese[] = { 0x3002, 0xFF0C, 0xFF1A, 0};
// Remark: Chinese (Taiwan) uses ascii punctuation marks though, but those
// don't have to be added, as only characters >= 128 will be wrapped.

wxString CWrapEngine::WrapText(wxWindow* parent, const wxString &text, unsigned long maxLength)
{
	/*
	This function wraps the given string so that it's width in pixels does 
	not exceed maxLength.
	In the general case, wrapping is done on word boundaries. Unfortunatly
	some languages, e.g. Chinese, don't separate words with spaces. In such
	languages it is allowed to break lines after any character.

	Though there are a few exceptions:
	- Don't wrap before punctuation marks
	- Wrap embedded English text fragments only on spaces
    */

	const wxChar* wrapAfter = 0;

	bool wrapOnEveryChar;

#if wxUSE_UNICODE
	// Just don't bother with wrapping on anything other than UCS-2
	// FIXME: Use charset conversion routines to convert into UCS-2 and back into
	//        local charset if not using unicode.
	int lang = wxGetApp().GetCurrentLanguage();
	if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED ||
		lang == wxLANGUAGE_CHINESE_TRADITIONAL || lang == wxLANGUAGE_CHINESE_HONGKONG ||
		lang == wxLANGUAGE_CHINESE_MACAU || lang == wxLANGUAGE_CHINESE_SINGAPORE ||
		lang == wxLANGUAGE_CHINESE_TAIWAN)
	{
		wrapOnEveryChar = true;
		wrapAfter = wrapAfter_Chinese;
	}
	else
#endif
		wrapOnEveryChar = false;

	wxString wrappedText;

	int width = 0, height = 0;

	const wxChar* str = text.c_str();
	// Scan entire string
	while (*str)
	{
		unsigned int lineLength = 0;

		const wxChar* p = str;

		// Remember unrwappable positions.
		const wxChar* minWrap = str;

		// Position of last space
		const wxChar* wrappable = 0;

		while (*p)
		{
			// Get width of all individual characters, record width of the current line
			parent->GetTextExtent(*p, &width, &height);
			lineLength += width;

			if (*p == '\n')
			{
				// Wrap on newline
				wrappedText += wxString(str, p - str + 1);
				str = p + 1;
				break;
			}
			else if (*p == ' ')
				// Remember position of last space
				wrappable = p;
			else if (wrapOnEveryChar && *p >= 128)
			{
				const wxChar* w = wrapAfter;
				while (*w)
				{
					if (*w == *p)
						break;

					w++;
				}
				if (!*w)
					wrappable = p;
			}

			if (lineLength > maxLength && wrappable)
			{
				wrappedText += wxString(str, wrappable - str) + _T("\n");
				if (*wrappable != ' ')
					str = wrappable;
				else
					str = wrappable + 1;
				break;
			}

			p++;
		}
		if (!*p)
		{
			wrappedText += str;
			break;
		}
	}

	return wrappedText;
}

bool CWrapEngine::WrapText(wxWindow* parent, int id, unsigned long maxLength)
{
	wxStaticText* pText = wxDynamicCast(parent->FindWindow(id), wxStaticText);
	if (!pText)
		return false;

	pText->SetLabel(WrapText(parent, pText->GetLabel(), maxLength));

	return true;
}

bool CWrapEngine::WrapRecursive(wxWindow* wnd, wxSizer* sizer, int max)
{
	// This function auto-wraps static texts.

	if (max <= 0)
		return false;

	wxPoint pos = sizer->GetPosition();

	for (unsigned int i = 0; i < sizer->GetChildren().GetCount(); i++)
	{
		wxSizerItem* item = sizer->GetItem(i);
		if (!item)
			continue;

		wxRect rect = item->GetRect();

		wxWindow* window;
		wxSizer* subSizer;
		if ((window = item->GetWindow()))
		{
			wxStaticText* text = wxDynamicCast(window, wxStaticText);
			if (!text)
				continue;

			wxString str = text->GetLabel();
			str = WrapText(text, str, max - rect.GetLeft() + pos.x);
			text->SetLabel(str);
		}
		else if ((subSizer = item->GetSizer()))
		{
			WrapRecursive(wnd, subSizer, wxMin(rect.GetWidth(), max - rect.GetLeft()));
		}
	}

	return true;
}

bool CWrapEngine::WrapRecursive(wxWindow* wnd, double ratio)
{
	std::vector<wxWindow*> windows;
	windows.push_back(wnd);
	return WrapRecursive(windows, ratio);
}

bool CWrapEngine::WrapRecursive(std::vector<wxWindow*>& windows, double ratio)
{
	wxSize size(0, 0);

	for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
	{
		wxSizer* pSizer = (*iter)->GetSizer();
		if (!pSizer)
			return false;

		size.IncTo(pSizer->GetMinSize());
	}

	double currentRatio = ((double)size.GetWidth() / size.GetHeight());
	if (ratio > currentRatio)
	{
		// Nothing to do, can't wrap anything
		return true;
	}

	int max = size.GetWidth();
	int min = size.GetHeight();
	int cur = (min + max) / 2;
	while (true)
	{
		wxSize size(0, 0);
		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
		{
			wxSizer* pSizer = (*iter)->GetSizer();
			WrapRecursive(*iter, pSizer, cur);
			pSizer->Layout();
			size.IncTo(pSizer->GetMinSize());
		}

		double newRatio = ((double)size.GetWidth() / size.GetHeight());

		if (newRatio < ratio)
			min = cur;
		else if (newRatio > ratio)
			max = cur;
		else
			break;

		cur = (min + max) / 2;
		if ((max - cur) < 10)
			break;
		if ((cur - min) < 10)
			break;

		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
			UnwrapRecursive(*iter, (*iter)->GetSizer());
		
		currentRatio = newRatio;
	}
	for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
	{
		wxSizer* pSizer = (*iter)->GetSizer();
		UnwrapRecursive(*iter, pSizer);
	
		WrapRecursive(*iter, pSizer, cur);
	}

	return true;
}

bool CWrapEngine::UnwrapRecursive(wxWindow* wnd, wxSizer* sizer)
{
	for (unsigned int i = 0; i < sizer->GetChildren().GetCount(); i++)
	{
		wxSizerItem* item = sizer->GetItem(i);
		if (!item)
			continue;

		wxWindow* window;
		wxSizer* subSizer;
		if ((window = item->GetWindow()))
		{
			wxStaticText* text = wxDynamicCast(window, wxStaticText);
			if (!text)
				continue;
			
			wxString str = text->GetLabel();
#if wxUSE_UNICODE
			int lang = wxGetApp().GetCurrentLanguage();
			if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED ||
				lang == wxLANGUAGE_CHINESE_TRADITIONAL || lang == wxLANGUAGE_CHINESE_HONGKONG ||
				lang == wxLANGUAGE_CHINESE_MACAU || lang == wxLANGUAGE_CHINESE_SINGAPORE ||
				lang == wxLANGUAGE_CHINESE_TAIWAN)
			{
				wxString unwrapped;
				const wxChar* p = str;
				bool wasAscii = false;
				while (*p)
				{
					if (*p == '\n')
					{
						if (wasAscii || *(p + 1) < 127)
							unwrapped += ' ';
					}
					else
						unwrapped += *p;

					if (*p < 127)
						wasAscii = true;
					else
						wasAscii = false;

					p++;
				}
				text->SetLabel(unwrapped);
			}
			else
#endif
			{
				str.Replace(_T("\n"), _T(" "));
				text->SetLabel(str);
			}
		}
		else if ((subSizer = item->GetSizer()))
		{
			UnwrapRecursive(wnd, subSizer);
		}
	}

	return true;
}
