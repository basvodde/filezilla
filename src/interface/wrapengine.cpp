#include "FileZilla.h"
#include "wrapengine.h"
#include <wx/wizard.h>
#include "filezillaapp.h"
#include "ipcmutex.h"
#include "xmlfunctions.h"
#include "buildinfo.h"

#define WRAPDEBUG 0
#if wxUSE_UNICODE
// Chinese equivalents to ".", "," and ":"
static const wxChar noWrapChars_Chinese[] = { '.', ',', ':', 0x3002, 0xFF0C, 0xFF1A, 0};
// Remark: Chinese (Taiwan) uses ascii punctuation marks though, but those
// don't have to be added, as only characters >= 128 will be wrapped.
#endif

bool CWrapEngine::CanWrapBefore(const wxChar& c)
{
	// Check if this is a punctuation character, we're not allowed
	// to wrap before such a character
	const wxChar* p = m_noWrapChars;
	while (*p)
	{
		if (*p == c)
			break;

		p++;
	}
	if (!*p)
		return true;

	return false;
}

bool CWrapEngine::WrapTextChinese(wxWindow* parent, wxString &text, unsigned long maxLength)
{
	wxASSERT(text.Find(_T("  ")) == -1);
	wxASSERT(text.Find(_T(" \n")) == -1);
	wxASSERT(text.Find(_T("\n ")) == -1);
	wxASSERT(text.Last() != ' ');
	wxASSERT(text.Last() != '\n');

	// See comment at start of WrapText function what this function does
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
				parent->GetTextExtent(*p, &width, &height, 0, 0, &m_font);
				if ((unsigned int)width > maxLength)
					return false;
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
				else if (*p >= 128)
				{
					if (CanWrapBefore(*p))
						wrappable = p;
				}
				else if (*(p - 1) >= 128 && CanWrapBefore(*p))
				{
					// Beginning of embedded English text, can wrap before
					wrappable = p;
				}
			}

			if (lineLength > maxLength && wrappable)
			{
				wxString tmp = wxString(str, wrappable - str);
				if (tmp.Last() == ' ')
					tmp.RemoveLast();
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
			if (lineLength > maxLength)
			{
				if (!wrappable)
					return false;

				const wxString& tmp = wxString(str, wrappable - str);
				wrappedText += tmp + _T("\n");
				if (*wrappable != ' ')
					str = wrappable;
				else
					str = wrappable + 1;
			}
			wrappedText += str;
			break;
		}
	}

#ifdef __WXDEBUG__
	wxString temp = wrappedText;
	wxASSERT(temp.Find(_T("  ")) == -1);
	wxASSERT(temp.Find(_T(" \n")) == -1);
	wxASSERT(temp.Find(_T("\n ")) == -1);
	wxASSERT(temp.Last() != ' ');
	wxASSERT(temp.Last() != '\n');
	temp.Replace(_T("&"), _T(""));
	while (temp != _T(""))
	{
		wxString piece;
		int pos = temp.Find(_T("\n"));
		if (pos == -1)
		{
			piece = temp;
			temp = _T("");
		}
		else
		{
			piece = temp.Left(pos);
			temp = temp.Mid(pos + 1);
		}
		parent->GetTextExtent(piece, &width, &height, 0, 0, &m_font);
		wxASSERT(width <= (int)maxLength);
	}
#endif

	text = wrappedText;

	return true;
}

bool CWrapEngine::WrapText(wxWindow* parent, wxString& text, unsigned long maxLength)
{
	/*
	This function wraps the given string so that it's width in pixels does
	not exceed maxLength.
	In the general case, wrapping is done on word boundaries. Thus we scan the
	string for spaces, measuer the length of the words and wrap if line becomes
	too long.
	It has to be done wordwise, as with some languages/fonts, the width in
	pixels of a line is smaller than the sum of the widths of every character.

	A special case are some languages, e.g. Chinese, which don't separate words
	with spaces. In such languages it is allowed to break lines after any
	character.

	Though there are a few exceptions:
	- Don't wrap before punctuation marks
	- Wrap embedded English text fragments only on spaces

	For this kind of languages, a different wrapping algorithm is used.
    */

	if (!m_font.IsOk())
		m_font = parent->GetFont();

	if (m_wrapOnEveryChar)
	{
#ifdef __WXDEBUG__
		const wxString original = text;
#endif
		bool res = WrapTextChinese(parent, text, maxLength);
#ifdef __WXDEBUG__
		wxString unwrapped = UnwrapText(text);
		wxASSERT(original == unwrapped);
#endif
		return res;
	}

	wxString wrappedText;

	int width = 0, height = 0;

	int spaceWidth = 0;
	parent->GetTextExtent(_T(" "), &spaceWidth, &height, 0, 0, &m_font);

	int strLen = text.Length();
	int wrapAfter = -1;
	int start = 0;
	unsigned int lineLength = 0;
	for (int i = 0; i <= strLen; i++)
	{
		if (text[i] != ' ' && text[i] != 0)
			continue;

		wxString segment;
		if (wrapAfter == -1)
		{
			segment = text.Mid(start, i - start);
			wrapAfter = i;
		}
		else
			segment = text.Mid(wrapAfter + 1, i - wrapAfter - 1);

		segment = wxStripMenuCodes(segment);
		parent->GetTextExtent(segment, &width, &height, 0, 0, &m_font);

		if ((unsigned int)width > maxLength)
		{
			// Something quite long. Perhaps it's a URL we can wrap at slashes?

			while (segment != _T(""))
			{
				// Find longest possible subsegment
				// which can be appended to current line, if there is a current line
				unsigned int j;
				int best = -1;

				for (j = 1; j < segment.Len() - 1; j++)
				{
					if (segment[j] == '/' && segment[j + 1] != '/')
					{
						wxString left = segment.Left(j + 1);
						int lwidth;
						parent->GetTextExtent(left, &lwidth, &height, 0, 0, &m_font);
						if ((unsigned)lwidth <= maxLength)
						{
							if (lineLength && lineLength + spaceWidth + lwidth > maxLength)
							{
								if (best == -1)
								{
									best = j;
									width = lwidth;
								}

								break;
							}

							best = j;
							width = lwidth;
						}
						else
							break;
					}
				}
				if (best == -1)
				{
					// No suitable slash found
					return false;
				}

				if (wrappedText != _T(""))
					wrappedText += _T("\n");

				if (lineLength)
					wrappedText += text.Mid(start, wrapAfter - start);

				if (lineLength && lineLength + spaceWidth + width > maxLength)
					wrappedText += _T("\n");
				else
					wrappedText += _T(" ");
				lineLength = 0;

				wrappedText += segment.Left(best + 1);
				segment = segment.Mid(best + 1);

				parent->GetTextExtent(wrappedText, &width, &height, 0, 0, &m_font);
				parent->GetTextExtent(segment, &width, &height);
				if ((unsigned)width <= maxLength)
				{
					break;
				}
			}

			start = i - segment.Len();
			wrapAfter = i;
			lineLength = 0;
		}

		if (lineLength + spaceWidth + width > maxLength)
		{
			if (wrappedText != _T(""))
				wrappedText += _T("\n");
			wrappedText += text.Mid(start, wrapAfter - start);
			if (width + spaceWidth >= (int)maxLength)
			{
				if (wrappedText != _T(""))
					wrappedText += _T("\n");
				wrappedText += text.Mid(wrapAfter + 1, i - wrapAfter - 1);

				start = i + 1;
				wrapAfter = -1;
				lineLength = 0;
			}
			else
			{
				start = wrapAfter + 1;
				wrapAfter = i;
				lineLength = width;
			}
		}
		else if (lineLength + spaceWidth + width + spaceWidth >= maxLength)
		{
			if (wrappedText != _T(""))
				wrappedText += _T("\n");
			wrappedText += text.Mid(start, i - start);
			if (text[i] != ' ')
				wrappedText += text[i];
			start = i + 1;
			wrapAfter = -1;
			lineLength = 0;
		}
		else
		{
			if (lineLength)
				lineLength += spaceWidth;
			lineLength += width;
			wrapAfter = i;
		}
	}
	if (start < strLen)
	{
		if (wrappedText != _T(""))
			wrappedText += _T("\n");
		wrappedText += text.Mid(start);
	}

	text = wrappedText;

	return true;
}

bool CWrapEngine::WrapText(wxWindow* parent, int id, unsigned long maxLength)
{
	wxStaticText* pText = wxDynamicCast(parent->FindWindow(id), wxStaticText);
	if (!pText)
		return false;

	wxString text = pText->GetLabel();
	if (!WrapText(parent, text, maxLength))
		return false;

	pText->SetLabel(text);

	return true;
}

#if WRAPDEBUG >= 3
	#define plvl { for (int i = 0; i < level; i++) printf(" "); }
#endif

bool CWrapEngine::WrapRecursive(wxWindow* wnd, wxSizer* sizer, int max)
{
	// This function auto-wraps static texts.
	
#if WRAPDEBUG >= 3
	static int level = 1;
    plvl printf("Enter\n");
#endif

	if (max <= 0)
	{
#if WRAPDEBUG >= 3
		plvl printf("Leave: max <= 0\n");
#endif
		return false;
	}

	for (unsigned int i = 0; i < sizer->GetChildren().GetCount(); i++)
	{
		wxSizerItem* item = sizer->GetItem(i);
		if (!item || !item->IsShown())
			continue;

		int rborder = 0;
		if (item->GetFlag() & wxRIGHT)
			rborder = item->GetBorder();
		int lborder = 0;
		if (item->GetFlag() & wxLEFT)
			lborder = item->GetBorder();

		wxRect rect = item->GetRect();

		wxSize min = item->GetMinSize();
		if (!min.IsFullySpecified())
			min = item->CalcMin();
		wxASSERT(min.GetWidth() + rborder + lborder <= sizer->GetMinSize().GetWidth());

		if (min.GetWidth() + item->GetPosition().x + lborder + rborder <= max)
		    continue;
			
		wxWindow* window;
		wxSizer* subSizer = 0;
		if ((window = item->GetWindow()))
		{
			wxStaticText* text = wxDynamicCast(window, wxStaticText);
			if (text)
			{
#ifdef __WXMAC__
				const int offset = 3;
#else
				const int offset = 2;
#endif
				if (max - rect.GetLeft() - rborder - offset <= 0)
					continue;
					
				wxString str = text->GetLabel();
				if (!WrapText(text, str, max - wxMax(0, rect.GetLeft()) - rborder - offset))
				{
#if WRAPDEBUG >= 3
					plvl printf("Leave: WrapText failed\n");
#endif				
					return false;
				}
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
					{
#if WRAPDEBUG >= 3
						plvl printf("Leave: WrapRecursive on notebook page failed\n");
#endif
						return false;
					}
				}
				continue;
			}
		}
		else if ((subSizer = item->GetSizer()))
		{
			int subBorder = 0;

			// Add border of static box sizer
			wxStaticBoxSizer* sboxSizer;
			if ((sboxSizer = wxDynamicCast(subSizer, wxStaticBoxSizer)))
			{
				int top, other;
				sboxSizer->GetStaticBox()->GetBordersForSizer(&top, &other);
				subBorder += other;
			}

#if WRAPDEBUG >= 3
			level++;
#endif
			bool res = WrapRecursive(0, subSizer, max - rborder - subBorder);
#if WRAPDEBUG >= 3
			level--;
#endif
			if (!res)
			{
#if WRAPDEBUG >= 3
				plvl printf("Leave: WrapRecursive on sizer failed\n");
#endif
				return false;
			}
		}
	}
	
#if WRAPDEBUG >= 3
	plvl printf("Leave: Success\n");
#endif


	return true;
}

bool CWrapEngine::WrapRecursive(wxWindow* wnd, double ratio, const char* name /*=""*/, wxSize canvas /*=wxSize()*/, wxSize minRequestedSize /*wxSize()*/)
{
	std::vector<wxWindow*> windows;
	windows.push_back(wnd);
	return WrapRecursive(windows, ratio, name, canvas, minRequestedSize);
}

bool CWrapEngine::WrapRecursive(std::vector<wxWindow*>& windows, double ratio, const char* name /*=""*/, wxSize canvas /*=wxSize()*/, wxSize minRequestedSize /*wxSize()*/)
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
			
			#ifdef __WXMAC__
			const int offset = 4;
#else
			const int offset = 0;
#endif

#ifdef __WXDEBUG__
			bool res =
#endif
			WrapRecursive(*iter, pSizer, maxWidth - offset);
			wxASSERT(res);
			pSizer->Layout();
			pSizer->Fit(*iter);
			wxSize size = pSizer->GetMinSize();
			wxASSERT(size.x <= maxWidth);
		}
		return true;
	}

	wxSize size = minRequestedSize;

	for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
	{
		wxSizer* pSizer = (*iter)->GetSizer();
		if (!pSizer)
			return false;

		pSizer->Layout();
		size.IncTo(pSizer->GetMinSize());
	}

	double currentRatio = ((double)(size.GetWidth() + canvas.x) / (size.GetHeight() + canvas.y));
	if (ratio >= currentRatio)
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

	double bestRatioDiff = currentRatio - ratio;
	int bestWidth = max;
	
#if WRAPDEBUG > 0	
	printf("Target ratio: %f\n", (float)ratio);
	printf("Canvas: % 4d % 4d\n", canvas.x, canvas.y);
#endif

	while (true)
	{

		wxSize size = minRequestedSize;
		for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
		{
			wxSizer* pSizer = (*iter)->GetSizer();
#ifdef __WXMAC__
			const int offset = 4;
#else
			const int offset = 0;
#endif
			WrapRecursive(*iter, pSizer, desiredWidth - offset);
			pSizer->Layout();
			size.IncTo(pSizer->GetMinSize());
		}

#if WRAPDEBUG > 0
	    printf("Current: % 4d % 4d   desiredWidth: %d\n", size.GetWidth(), size.GetHeight(), desiredWidth);
#endif
		if (size.GetWidth() > desiredWidth)
		{
			// Wrapping failed
			min = desiredWidth;
			if (max - min < 5)
				break;
			desiredWidth = (min + max) / 2;

			for (std::vector<wxWindow*>::iterator iter = windows.begin(); iter != windows.end(); iter++)
			{
				UnwrapRecursive(*iter, (*iter)->GetSizer());
				(*iter)->GetSizer()->Layout();
			}
			continue;
		}
		actualWidth = size.GetWidth();

		double newRatio = ((double)(size.GetWidth() + canvas.x) / (size.GetHeight() + canvas.y));
#if WRAPDEBUG > 0		
		printf("Ratio: %f\n", (float)newRatio);
#endif		

		if (newRatio < ratio)
		{
			if (ratio - newRatio < bestRatioDiff)
			{
				bestRatioDiff = ratio - newRatio;
				bestWidth = actualWidth;
			}

			if (min >= actualWidth)
				min = desiredWidth;
			else
				min = actualWidth;
		}
		else if (newRatio > ratio)
		{
			if (newRatio - ratio < bestRatioDiff)
			{
				bestRatioDiff = newRatio - ratio;
				bestWidth = actualWidth;
			}

			if (max == actualWidth)
				break;
			max = actualWidth;
		}
		else
		{
			bestRatioDiff = ratio - newRatio;
			bestWidth = actualWidth;
			break;
		}

		if (max - min < 2)
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

		WrapRecursive(*iter, pSizer, bestWidth);
		pSizer->Layout();
		pSizer->Fit(*iter);
		size = pSizer->GetMinSize();
		wxASSERT(size.x <= bestWidth);
	}

	SetWidthToCache(name, bestWidth);

	return true;
}

wxString CWrapEngine::UnwrapText(const wxString& text)
{
	wxString unwrapped;
#if wxUSE_UNICODE
	int lang = wxGetApp().GetCurrentLanguage();
	if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED ||
		lang == wxLANGUAGE_CHINESE_TRADITIONAL || lang == wxLANGUAGE_CHINESE_HONGKONG ||
		lang == wxLANGUAGE_CHINESE_MACAU || lang == wxLANGUAGE_CHINESE_SINGAPORE ||
		lang == wxLANGUAGE_CHINESE_TAIWAN)
	{
		const wxChar* p = text;
		bool wasAscii = false;
		while (*p)
		{
			if (*p == '\n')
			{
				if (wasAscii)
					unwrapped += ' ';
				else if (*(p + 1) < 127)
				{
					if ((*(p + 1) != '(' || *(p + 2) != '&') && CanWrapBefore(*(p - 1)))
						unwrapped += ' ';
				}
			}
			else if (*p != '\r')
				unwrapped += *p;

			if (*p < 127)
				wasAscii = true;
			else
				wasAscii = false;

			p++;
		}
	}
	else
#endif
	{
		unwrapped = text;
		unwrapped.Replace(_T("\n"), _T(" "));
		unwrapped.Replace(_T("\r"), _T(""));
	}
	return unwrapped;
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
				wxString unwrapped = UnwrapText(text->GetLabel());
                text->SetLabel(unwrapped);

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
	if (!name || !*name)
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

	wxString language = wxGetApp().GetCurrentLanguageCode();

	TiXmlElement* pLanguage = FindElementWithAttribute(pElement, "Language", "id", language.mb_str());
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
	if (!name || !*name)
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

	wxString language = wxGetApp().GetCurrentLanguageCode();

	TiXmlElement* pLanguage = FindElementWithAttribute(pElement, "Language", "id", language.mb_str());
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
	CheckLanguage();
}

CWrapEngine::~CWrapEngine()
{
}

static wxString GetLocaleFile(const wxString& localesDir, wxString name)
{
	if (wxFileName::FileExists(localesDir + name + _T("/filezilla.mo")))
		return name;
	if (wxFileName::FileExists(localesDir + name + _T("/LC_MESSAGES/filezilla.mo")))
		return name + _T("/LC_MESSAGES");

	size_t pos = name.Find('@');
	if (pos > 0)
	{
		name = name.Left(pos);
		if (wxFileName::FileExists(localesDir + name + _T("/filezilla.mo")))
			return name;
		if (wxFileName::FileExists(localesDir + name + _T("/LC_MESSAGES/filezilla.mo")))
			return name + _T("/LC_MESSAGES");
	}

	pos = name.Find('_');
	if (pos > 0)
	{
		name = name.Left(pos);
		if (wxFileName::FileExists(localesDir + name + _T("/filezilla.mo")))
			return name;
		if (wxFileName::FileExists(localesDir + name + _T("/LC_MESSAGES/filezilla.mo")))
			return name + _T("/LC_MESSAGES");
	}

	return _T("");
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

	const wxString buildDate = CBuildInfo::GetBuildDateString();
	if (GetTextAttribute(pElement, "Builddate") != buildDate)
	{
		cacheValid = false;
		SetTextAttribute(pElement, "Builddate", buildDate);
	}

	const wxString buildTime = CBuildInfo::GetBuildTimeString();
	if (GetTextAttribute(pElement, "Buildtime") != buildTime)
	{
		cacheValid = false;
		SetTextAttribute(pElement, "Buildtime", buildTime);
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

	// Get current language
	wxString language = wxGetApp().GetCurrentLanguageCode();
	if (language == _T(""))
		language = _T("default");

	TiXmlElement* languageElement = FindElementWithAttribute(pElement, "Language", "id", language.mb_str());
	if (!languageElement)
	{
		languageElement = pElement->InsertEndChild(TiXmlElement("Language"))->ToElement();
		languageElement->SetAttribute("id", language.mb_str());
	}

	// Get static text font and measure sample text
	wxFrame* pFrame = new wxFrame;
	pFrame->Create(0, -1, _T("Title"), wxDefaultPosition, wxDefaultSize, wxFRAME_TOOL_WINDOW);
	wxStaticText* pText = new wxStaticText(pFrame, -1, _T("foo"));

	wxFont font = pText->GetFont();
	wxString fontDesc = font.GetNativeFontInfoDesc();

	TiXmlElement* pFontElement = languageElement->FirstChildElement("Font");
	if (!pFontElement)
		pFontElement = languageElement->InsertEndChild(TiXmlElement("Font"))->ToElement();

	if (GetTextAttribute(pFontElement, "font") != fontDesc)
	{
		SetTextAttribute(pFontElement, "font", fontDesc);
		cacheValid = false;
	}

	int width, height;
	pText->GetTextExtent(_T("Just some test string we are measuring. If width or heigh differ from the recorded values, invalidate cache. 1234567890MMWWII"), &width, &height);

	if (GetAttributeInt(pFontElement, "width") != width ||
		GetAttributeInt(pFontElement, "height") != height)
	{
		cacheValid = false;
		SetAttributeInt(pFontElement, "width", width);
		SetAttributeInt(pFontElement, "height", height);
	}

	pFrame->Destroy();

	// Get language file
	const wxString& localesDir = wxGetApp().GetLocalesDir();
	wxString name = GetLocaleFile(localesDir, language);

	if (name != _T(""))
	{
		wxFileName fn(localesDir + name + _T("/filezilla.mo"));
		wxDateTime date = fn.GetModificationTime();
		wxLongLong ticks = date.GetTicks();

		const char* languageNodeDate = languageElement->Attribute("date");
		if (!languageNodeDate || strcmp(languageNodeDate, ticks.ToString().mb_str()))
		{
			languageElement->SetAttribute("date", ticks.ToString().mb_str());
			cacheValid = false;
		}
	}
	else
		languageElement->SetAttribute("date", "");
	if (!cacheValid)
	{
		TiXmlElement* dialog;
		while ((dialog = languageElement->FirstChildElement("Dialog")))
			languageElement->RemoveChild(dialog);
	}

	wxString error;
	if (!SaveXmlFile(file, pDocument, &error))
	{
		wxString msg = wxString::Format(_("Could not write \"%s\": %s"), file.GetFullPath().c_str(), error.c_str());
		wxMessageBox(msg, _("Error writing xml file"), wxICON_ERROR);
	}

	delete pDocument->GetDocument();

	return true;
}

void CWrapEngine::ClearCache()
{
	// We have to synchronize access to layout.xml so that multiple processed don't write
	// to the same file or one is reading while the other one writes.
	CInterProcessMutex mutex(MUTEX_LAYOUT);

	wxFileName file(wxGetApp().GetSettingsDir(), _T("layout.xml"));
	if (file.FileExists())
		wxRemoveFile(file.GetFullPath());
}

void CWrapEngine::CheckLanguage()
{
	#if wxUSE_UNICODE
	// Just don't bother with wrapping on anything other than UCS-2
	// FIXME: Use charset conversion routines to convert into UCS-2 and back into
	//        local charset if not using unicode.
	int lang = wxGetApp().GetCurrentLanguage();
	if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED ||
		lang == wxLANGUAGE_CHINESE_TRADITIONAL || lang == wxLANGUAGE_CHINESE_HONGKONG ||
		lang == wxLANGUAGE_CHINESE_MACAU || lang == wxLANGUAGE_CHINESE_SINGAPORE ||
		lang == wxLANGUAGE_CHINESE_TAIWAN ||
		lang == wxLANGUAGE_JAPANESE)
	{
		m_wrapOnEveryChar = true;
		m_noWrapChars = noWrapChars_Chinese;
	}
	else
#endif
	{
		m_wrapOnEveryChar = false;
		m_noWrapChars = 0;
	}
}
