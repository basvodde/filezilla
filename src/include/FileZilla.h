#ifndef __FILEZILLA_H__
#define __FILEZILLA_H__

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "FileZilla 3"
#endif

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FileZilla"
#endif

#ifndef PACKAGE_VERSION
	#define PACKAGE_VERSION "custom build"

	// Disable updatechecks if we have no version information
	#ifdef FZ_MANUALUPDATECHECK
		#undef FZ_MANUALUPDATECHECK
		#define FZ_MANUALUPDATECHECK 0
	#endif
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

#include <wx/wx.h>

// Include after wx.h so that __WXFOO__ is properly defined
#include "setup.h"

#include <wx/splitter.h>
#include <wx/wxhtml.h>
#include <wx/socket.h>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/sysopt.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/mimetype.h>
#include <wx/dir.h>
#include <wx/list.h>
#include <wx/filefn.h>
#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/filefn.h>
#include <wx/file.h>

#include "compatibility.h"

#include <list>
#include <vector>
#include <map>

// Enhancements to classes provided by wxWidgets
#include "timeex.h"
#include "threadex.h"

#include "optionsbase.h"
#include "logging.h"
#include "server.h"
#include "serverpath.h"
#include "commands.h"
#include "notification.h"
#include "FileZillaEngine.h"
#include "directorylisting.h"

#include "misc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#endif
