#ifndef __OPTIONSPAGE_H__
#define __OPTIONSPAGE_H__

class COptions;
class CSettingsDialog;
class COptionsPage : public wxPanel
{
public:
	bool CreatePage(COptions* pOptions, wxWindow* parent, wxSize& maxSize);

	virtual wxString GetResourceName() = 0;
	virtual bool LoadPage() = 0;
	virtual bool SavePage() = 0;
	virtual bool Validate() { return true; }

protected:
	COptions* m_pOptions;
};

#endif //__OPTIONSPAGE_H__
