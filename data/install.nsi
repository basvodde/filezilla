;NSIS Modern User Interface version 1.70
;FileZilla 3 installation script
;Written by Tim Kosse <mailto:tim.kosse@gmx.de>

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "FileZilla 3 alpha 2"
  OutFile "FileZilla_3_alpha2_setup.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\FileZilla 3"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\FileZilla 3" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "..\COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\FileZilla 3" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "FileZilla 3"
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "FileZilla 3"
  
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "FileZilla 3" SecMain

  SetOutPath "$INSTDIR"
  
  File "..\src\interface\FileZilla.exe"
  File "mingwm10.dll"
  File "..\GPL.html"

  SetOutPath "$INSTDIR\resources"
  File "..\src\interface\resources\*.xrc"
  File "..\src\interface\resources\*.png"
  File "..\src\interface\resources\*.xpm"

  SetOutPath "$INSTDIR\resources\16x16"
  File "..\src\interface\resources\16x16\*.png"

  SetOutPath "$INSTDIR\resources\32x32"
  File "..\src\interface\resources\32x32\*.png"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\FileZilla 3" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\FileZilla 3.lnk" "$INSTDIR\FileZilla 3.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section "Language files" SecLang

  SetOutPath "$INSTDIR\locales\de\LC_MESSAGES"
  File /oname=filezilla.mo "..\locales\de.mo"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "Required program files."
  LangString DESC_SecLang ${LANG_ENGLISH} "Language files files."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecLang} $(DESC_SecLang)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\FileZilla.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "mingwm10.dll"
  Delete "$INSTDIR\GPL.html"
  Delete "$INSTDIR\resources\*.xrc"
  Delete "$INSTDIR\resources\*.png"
  Delete "$INSTDIR\resources\*.xpm"
  Delete "$INSTDIR\resources\16x16\*.png"
  Delete "$INSTDIR\resources\32x32\*.png"

  Delete "$INSTDIR\locales\de\LC_MESSAGES\filezilla.mo"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR\locales\de\LC_MESSAGES"
  RMDir "$INSTDIR\locales\de"
  RMDir "$INSTDIR\locales"
  RMDir "$INSTDIR\resources\32x32"
  RMDir "$INSTDIR\resources\16x16"
  RMDir "$INSTDIR\resources"
  RMDir "$INSTDIR"

  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\FileZilla 3.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\FileZilla 3"

SectionEnd