#include "FileZilla.h"
#include "queue.h"
#include "queueview_failed.h"

CQueueViewFailed::CQueueViewFailed(wxAuiNotebookEx* parent, int id)
	: CQueueViewBase(parent, id)
{
	CreateColumns(_("Reason"));
}
