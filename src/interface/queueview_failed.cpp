#include "FileZilla.h"
#include "queue.h"
#include "queueview_failed.h"

CQueueViewFailed::CQueueViewFailed(CQueue* parent, int index)
	: CQueueViewBase(parent, index, _("Failed transfers"))
{
	CreateColumns(_("Reason"));
}
