#include "FileZilla.h"
#include "wrapengine.h"
#include <wx/wizard.h>
#include "filezillaapp.h"
#include "ipcmutex.h"
#include "xmlfunctions.h"
#include "buildinfo.h"

#if wxUSE_UNICODE
// Chinese equivalents to ".", "," and ":"
static const wxChar wrapAfter_Chinese[] = { 0x3002, 0xFF0C, 0xFF1A, 0};
// Remark: Chinese (Taiwan) uses ascii punctuation marks though, but those
// don't have to be added, as only characters >= 128 will be wrapped.
#endif

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

		// Position of last wrappable character
		const wxChar* wrappable = 0;

		bool lastAmpersand = false;
		while (*p)
		{
			if (*p == '&')
			{
				if (!lastAmpersand)
				{
					lastAmpersand = true;
					p++;
					continue;
				}
				else
					lastAmpersand = false;
			}
			std::map<wxChar, unsigned int>::const_iterator iter = m_charWidths.find(*p);
			if (iter == m_charWidths.end())
			{
				// Get width of all individual characters, record width of the current line
				parent->GetTextExtent(*p, &width, &height);
				m_charWidths[*p] = width;
				lineLength += width;
			}
			else
				lineLength += iter->second;

			wxASSERT(*p != '\r');
			if (*p == '\n')
			{
				// Wrap on newline
				wrappedText += wxString(str, p - str + 1);
				str = p + 1;
				break;
			}
			else if (p != str) // Don't wrap at first character
			{
				if (*p == ' ')
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
			}

			if (lineLength > maxLength && wrappable)
			{
				const wxString& tmp = wxString(str, wrappable - str);
				wrappedText += tmp + _T("\n");
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

	const wxPoint& pos = sizer->GetPosition();
	const wxSize& sizerSize = sizer->GetSize();

	for (unsigned int i = 0; i < sizer->GetChildren().GetCount(); i++)
	{
		wxSizerItem* item = sizer->GetItem(i);
		if (!item)
			continue;

		int rborder = 0;
		if (item->GetFlag() & wxRIGHT)
			rborder = item->GetBorder();
		int lborder = 0;
		if (item->GetFlag() & wxLEFT)
			lborder = item->GetBorder();

		wxRect rect = item->GetRect();
#ifdef __WXMSW__
		wxASSERT(rect.GetRight() + rborder < pos.x + sizerSize.GetWidth());
#endif
		wxASSERT(item->GetMinSize().GetWidth() + rborder + lborder <= sizer->GetMinSize().GetWidth());

		if (item->GetMinSize().GetWidth() + item->GetPosition().x + rborder <= max)
		    continue;

		wxWindow* window;
		wxSizer* subSizer;
		if ((window = item->GetWindow()))
		{
			wxStaticText* text = wxDynamicCast(window, wxStaticText);
			if (text)
			{
				wxString str = text->GetLabel();
				str = WrapText(text, str,  max - rect.GetLeft() - rborder - 2);
				text->SetLabel(str);

				continue;
			}

			wxNotebook* book = wxDynamicCast(window, wxNotebook);
			if (book)
			{
				int maxPageWidth = 0;
				for (unsigned int i = 0; i < book->GetPageCount(); i++)
				{
					wxNotebookPage* page = book->GetPage(i);
					maxPageWidth = wxMax(maxPageWidth, page->GetRect().GetWidth());
				}

				for (unsigned int i = 0; i < book->GetPageCount(); i++)
				{
					wxNotebookPage* page = book->GetPage(i);
					wxRect pageRect = page->GetRect();
					int pageMax = max - rect.GetLeft() - pageRect.GetLeft() - rborder - rect.GetWidth() + maxPageWidth;
					if (!WrapRecursive(wnd, page->GetSizer(), pageMax))
						return false;
				}
				continue;
			}

			if (item->GetMinSize().GetWidth() + item->GetPosition().x + rborder > max)
				return false;
		}
		else if ((subSizer = item->GetSizer()))
		{
			// FIXME: wxGridSizer not implemented
			wxASSERT(wxDynamicCast(subSizer, wxFlexGridSizer) || !wxDynamicCast(subSizer, wxGridSizer));

			int subBorder = 0;

			// Add border of static box sizer
			wxStaticBoxSizer* sboxSizer;
			if ((sboxSizer = wxDynamicCast(subSizer, wxStaticBoxSizer)))
			{
				int top, other;
				sboxSizer->GetStaticBox()->GetBordersForSizer(&top, &other);
				subBorder += other;
			}

			if (!WrapRecursive(0, subSizer, max - rborder - subBorder))
				return false;
		}
	}

	return true;
}

bool CWrapEngine::WrapRecursive(wxWindow* wnd, double ratio, const char* name /*=""*/, wxSize canvas /*=wxSize()*/)
{
	std::vector<wxWindow*> windows;
	windows.push_back(wnd);
	return WrapRecursive(windows, ratio, name, canvas);
}

bool CWrapEngine::WrapRecursive(std::vector<wxWindow*>& windows, double ratio, const char* name /*=""*/, wxSize canvas /*=wxSize()*/)
{
	int maxWidth = GetWidthFromCache(name);
	if (maxWidth)
	{
		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
		{
			wxSizer* pSizer = (*iter)->GetSizer();
			if (!pSizer)
				continue;

			pSizer->Layout();
			WrapRecursive(*iter, pSizer, maxWidth);
			pSizer->Layout();
			pSizer->Fit(*iter);
		}
		return true;
	}

	wxSize size(0, 0);

	for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
	{
		wxSizer* pSizer = (*iter)->GetSizer();
		if (!pSizer)
			return false;

		pSizer->Layout();
		size.IncTo(pSizer->GetMinSize());
	}

	double currentRatio = ((double)(size.GetWidth() + canvas.x) / (size.GetHeight() + canvas.y));
	if (ratio > currentRatio)
	{
		// Nothing to do, can't wrap anything
		return true;
	}

	int max = size.GetWidth();
	int min = wxMin(size.GetWidth(), size.GetHeight());
	if (ratio < 0)
		min = (int)(min * ratio);
	if (min > canvas.x)
		min -= canvas.x;
	int desiredWidth = (min + max) / 2;
	int actualWidth = size.GetWidth();
	while (true)
	{
		wxSize size(0, 0);
		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
		{
			wxSizer* pSizer = (*iter)->GetSizer();
			WrapRecursive(*iter, pSizer, desiredWidth);
			pSizer->Layout();
			size.IncTo(pSizer->GetMinSize());
		}

		if (size.GetWidth() > desiredWidth)
		{
			// Wrapping failed
			min = desiredWidth;
			if (max - min < 5)
				break;
			desiredWidth = (min + max) / 2;
			continue;
		}
		actualWidth = size.GetWidth();

		double newRatio = ((double)(size.GetWidth() + canvas.x) / (size.GetHeight() + canvas.y));

		if (newRatio < ratio)
		{
			if (min == actualWidth)
				break;
			min = actualWidth;
		}
		else if (newRatio > ratio)
		{
			if (max == actualWidth)
				break;
			max = actualWidth;
		}
		else
			break;

		if (max - min < 5)
			break;
		desiredWidth = (min + max) / 2;

		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
		{
			UnwrapRecursive(*iter, (*iter)->GetSizer());
			(*iter)->GetSizer()->Layout();
		}

		currentRatio = newRatio;
	}
	for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
	{
		wxSizer* pSizer = (*iter)->GetSizer();
		UnwrapRecursive(*iter, pSizer);
		pSizer->Layout();

		WrapRecursive(*iter, pSizer, max);
		pSizer->Layout();
		pSizer->Fit(*iter);
		size = pSizer->GetMinSize();
		wxASSERT(size.x <= max);
	}

	SetWidthToCache(name, max);

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
			if (text)
			{
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
						else if (*p != '\r')
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
					str.Replace(_T("\r"), _T(""));
					text->SetLabel(str);
				}
				continue;
			}

			wxNotebook* book = wxDynamicCast(window, wxNotebook);
			if (book)
			{
				for (unsigned int i = 0; i < book->GetPageCount(); i++)
				{
					wxNotebookPage* page = book->GetPage(i);
					UnwrapRecursive(wnd, page->GetSizer());
				}
				continue;
			}
		}
		else if ((subSizer = item->GetSizer()))
		{
			UnwrapRecursive(wnd, subSizer);
		}
	}

	return true;
}

int CWrapEngine::GetWidthFromCache(const char* name)
{
	if (!name || name == "")
		return 0;

	// We have to synchronize access to layout.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_LAYOUT);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("layout.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);

	if (!pDocument)
		return 0;

	TiXmlElement* pElement = pDocument->FirstChildElement("Layout");
	if (!pElement)
	{
		delete pDocument->GetDocument();
		return 0;
	}

	int language = wxGetApp().GetCurrentLanguage();

	TiXmlElement* pLanguage = FindElementWithAttribute(pElement, "Language", "id", language);
	if (!pLanguage)
	{
		delete pDocument->GetDocument();
		return 0;
	}

	TiXmlElement* pDialog = FindElementWithAttribute(pLanguage, "Dialog", "name", name);
	if (!pDialog)
	{
		delete pDocument->GetDocument();
		return 0;
	}

	int value = GetAttributeInt(pDialog, "width");

	delete pDocument->GetDocument();

	return value;
}

void CWrapEngine::SetWidthToCache(const char* name, int width)
{
	if (!name || name == "")
		return;

	// We have to synchronize access to layout.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_LAYOUT);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("layout.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);

	if (!pDocument)
		return;

	TiXmlElement* pElement = pDocument->FirstChildElement("Layout");
	if (!pElement)
	{
		delete pDocument->GetDocument();
		return;
	}

	int language = wxGetApp().GetCurrentLanguage();

	TiXmlElement* pLanguage = FindElementWithAttribute(pElement, "Language", "id", language);
	if (!pLanguage)
	{
		delete pDocument->GetDocument();
		return;
	}

	TiXmlElement* pDialog = FindElementWithAttribute(pLanguage, "Dialog", "name", name);
	if (!pDialog)
	{
		pDialog = pLanguage->InsertEndChild(TiXmlElement("Dialog"))->ToElement();
		pDialog->SetAttribute("name", name);
	}

	pDialog->SetAttribute("width", width);
	wxString error;
	SaveXmlFile(file, pDocument, &error);

	delete pDocument->GetDocument();
}

CWrapEngine::CWrapEngine()
{
}

CWrapEngine::~CWrapEngine()
{
}

bool CWrapEngine::LoadCache()
{
	// We have to synchronize access to layout.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_LAYOUT);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("layout.xml"));
	TiXmlElement* pDocument = GetXmlFile(file);

	if (!pDocument)
	{
		wxString msg = wxString::Format(_("Could not load \"%s\", please make sure the file is valid and can be accessed."), file.GetFullPath().c_str());
		wxMessageBox(msg, _("Error loading xml file"), wxICON_ERROR);

		return false;
	}

	bool cacheValid = true;

	TiXmlElement* pElement = pDocument->FirstChildElement("Layout");
	if (!pElement)
		pElement = pDocument->InsertEndChild(TiXmlElement("Layout"))->ToElement();

	const wxString buildDate = CBuildInfo::GetBuildDate();
	if (GetTextAttribute(pElement, "Builddate") != buildDate)
	{
		cacheValid = false;
		SetTextAttribute(pElement, "Builddate", buildDate);
	}

	// Enumerate resource file names
	// -----------------------------

	TiXmlElement* pResources = pElement->FirstChildElement("Resources");
	if (!pResources)
		pResources = pElement->InsertEndChild(TiXmlElement("Resources"))->ToElement();

	wxString resourceDir = wxGetApp().GetResourceDir();
	wxDir dir(resourceDir);

	wxLogNull log;

	wxString xrc;
	for (bool found = dir.GetFirst(&xrc, _T("*.xrc")); found; found = dir.GetNext(&xrc))
	{
		if (!wxFileName::FileExists(resourceDir + xrc))
			continue;

		wxFileName fn(resourceDir + xrc);
		wxDateTime date = fn.GetModificationTime();
		wxLongLong ticks = date.GetTicks();

		TiXmlElement* resourceElement = FindElementWithAttribute(pResources, "xrc", "file", xrc.mb_str());
		if (!resourceElement)
		{
			resourceElement = pResources->InsertEndChild(TiXmlElement("xrc"))->ToElement();
			resourceElement->SetAttribute("file", xrc.mb_str());
			resourceElement->SetAttribute("date", ticks.ToString().mb_str());
			cacheValid = false;
		}
		else
		{
			const char* xrcNodeDate = resourceElement->Attribute("date");
			if (!xrcNodeDate || strcmp(xrcNodeDate, ticks.ToString().mb_str()))
			{
				cacheValid = false;

				resourceElement->SetAttribute("date", ticks.ToString().mb_str());
			}
		}
	}

	// Get static text font and measure sample text
	wxFrame* pFrame = new wxFrame;
	pFrame->Create(0, -1, _T("Title"), wxDefaultPosition, wxDefaultSize, wxFRAME_TOOL_WINDOW);
	wxStaticText* pText = new wxStaticText(pFrame, -1, _T("foo"));

	wxFont font = pText->GetFont();
	wxString fontDesc = font.GetNativeFontInfoDesc();

	TiXmlElement* pFontElement = pElement->FirstChildElement("Font");
	if (!pFontElement)
		pFontElement = pElement->InsertEndChild(TiXmlElement("Font"))->ToElement();

	if (GetTextAttribute(pFontElement, "font") != fontDesc)
	{
		SetTextAttribute(pFontElement, "font", fontDesc);
		cacheValid = false;
	}

	int width, height;
	pText->GetTextExtent(_T("Just some test string we are measuring. If width or heigh differ from the recorded values, invalidate cache."), &width, &height);

	if (GetAttributeInt(pFontElement, "width") != width ||
		GetAttributeInt(pFontElement, "height") != height)
	{
		cacheValid = false;
		SetAttributeInt(pFontElement, "width", width);
		SetAttributeInt(pFontElement, "height", height);
	}

	pFrame->Destroy();


	if (!cacheValid)
	{
		// Clear all languages
		TiXmlElement* pLanguage = pElement->FirstChildElement("Language");
		while (pLanguage)
		{
			pElement->RemoveChild(pLanguage);
			pLanguage = pElement->FirstChildElement("Language");
		}
	}

	// Enumerate language files
	// ------------------------

	const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(_T("en"));
	if (pInfo)
	{
		TiXmlElement* languageElement = FindElementWithAttribute(pElement, "Language", "id", pInfo->Language);
		if (!languageElement)
		{
			languageElement = pElement->InsertEndChild(TiXmlElement("Language"))->ToElement();
			languageElement->SetAttribute("id", pInfo->Language);
		}
	}


	wxString localesDir = wxGetApp().GetLocalesDir();
	dir.Open(localesDir);

	wxString locale;
	for (bool found = dir.GetFirst(&locale); found; found = dir.GetNext(&locale))
	{
		if (!wxFileName::FileExists(localesDir + locale + _T("/filezilla.mo")))
			continue;

		wxString name;
		const wxLanguageInfo* pInfo = wxLocale::FindLanguageInfo(locale);
		if (!pInfo)
			continue;

		wxFileName fn(localesDir + locale + _T("/filezilla.mo"));
		wxDateTime date = fn.GetModificationTime();
		wxLongLong ticks = date.GetTicks();

		TiXmlElement* languageElement = FindElementWithAttribute(pElement, "Language", "id", pInfo->Language);
		if (!languageElement)
		{
			languageElement = pElement->InsertEndChild(TiXmlElement("Language"))->ToElement();
			languageElement->SetAttribute("id", pInfo->Language);
			languageElement->SetAttribute("date", ticks.ToString().mb_str());
		}
		else
		{
			const char* languageNodeDate = languageElement->Attribute("date");
			if (!languageNodeDate || strcmp(languageNodeDate, ticks.ToString().mb_str()))
			{
				languageElement->SetAttribute("date", ticks.ToString().mb_str());
				languageElement->Clear();
			}
		}
	}

	// Outdated cache entries are now purged

	wxString error;
	if (!SaveXmlFile(file, pDocument, &error))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\": %s"), file.GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	delete pDocument->GetDocument();

	return true;
}
