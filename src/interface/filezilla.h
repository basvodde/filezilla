#ifndef __FILEZILLA_H__
#define __FILEZILLA_H__

#include <libfilezilla.h>

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FileZilla"
#endif

#ifndef PACKAGE_VERSION
	#define PACKAGE_VERSION "custom build"

	// Disable updatechecks if we have no version information
	#ifdef FZ_MANUALUPDATECHECK
		#undef FZ_MANUALUPDATECHECK
	#endif
	#define FZ_MANUALUPDATECHECK 0
#endif

#ifndef FZ_MANUALUPDATECHECK
	#define FZ_MANUALUPDATECHECK 1
#endif

#ifndef FZ_AUTOUPDATECHECK
	#if FZ_MANUALUPDATECHECK
		#define FZ_AUTOUPDATECHECK 1
	#else
		#define FZ_AUTOUPDATECHECK 0
	#endif
#else
	#if FZ_AUTOUPDATECHECK && !FZ_MANUALUPDATECHECK
		#undef FZ_AUTOUPDATECHECK
		#define FZ_AUTOUPDATECHECK 0
	#endif
#endif

#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/dataobj.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/dialog.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/frame.h>
#include <wx/list.h>
#include <wx/listctrl.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/mimetype.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/spinctrl.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>
#include <wx/sysopt.h>
#include <wx/textdlg.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

#endif
