#ifndef __EXPORT_H__
#define __EXPORT_H__

#include "dialogex.h"

class CQueueView;
class CExportDialog : protected wxDialogEx
{
public:
	CExportDialog(wxWindow* parent, CQueueView* pQueueView);

	void Show();

protected:
	wxWindow* const m_parent;
	const CQueueView* const m_pQueueView;
};

#endif //__EXPORT_H__
