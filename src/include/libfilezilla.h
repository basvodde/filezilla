#ifndef __LIBFILEZILLA_H__
#define __LIBFILEZILLA_H__

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "FileZilla 3"
#endif

#include <wx/defs.h>

// Include after defs.h so that __WXFOO__ is properly defined
#include "setup.h"

#ifdef __WXMSW__
#include <windows.h>
#endif

#include <wx/datetime.h>
#include <wx/event.h>
#include <wx/string.h>

#include <list>
#include <vector>
#include <map>

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

#endif //__LIBFILEZILLA_H__
