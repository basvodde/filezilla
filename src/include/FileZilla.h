#ifndef __FILEZILLA_H__
#define __FILEZILLA_H__

#ifdef HAVE_CONFIG_H
  #include <config.h>
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

#include <list>
#include <vector>
#include <map>

#include "optionsbase.h"
#include "logging.h"
#include "server.h"
#include "serverpath.h"
#include "commands.h"
#include "notification.h"
#include "timeex.h"
#include "FileZillaEngine.h"
#include "directorylisting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#endif
