
bin_PROGRAMS = filezilla_engine_tests filezilla_interface_tests

TESTS = $(bin_PROGRAMS)

WX_STUBS = stubs/wx/stubWxMsgdlg.cpp stubs/wx/stubWxGeometry.cpp stubs/wx/stubWxCommandEvent.cpp stubs/wx/stubWxWindow.cpp stubs/wx/stubWxFrame.cpp stubs/wx/stubWxColour.cpp stubs/wx/stubWxFont.cpp stubs/wx/stubWxGDIObject.cpp stubs/wx/stubWxRegion.cpp stubs/wx/stubWxPalette.cpp stubs/wx/stubWxAccel.cpp stubs/wx/stubWxBrush.cpp stubs/wx/stubWxCursor.cpp stubs/wx/stubWxBitmap.cpp stubs/wx/stubWxImage.cpp stubs/wx/stubWxTextCtrl.cpp stubs/wx/stubWxMenuItem.cpp stubs/wx/stubWxWizard.cpp stubs/wx/stubWxAuiNotebook.cpp stubs/wx/stubWxSound.cpp stubs/wx/stubWxComboBox.cpp stubs/wx/stubWxXmlRes.cpp stubs/wx/stubWxApp.cpp stubs/wx/stubWxArtProv.cpp stubs/wx/stubWxTreeCtrl.cpp stubs/wx/stubWxSplitter.cpp stubs/wx/stubWxDialog.cpp stubs/wx/stubWxEvent.cpp stubs/wx/stubWxTimer.cpp stubs/wx/stubWxCtrlSub.cpp stubs/wx/stubWxControl.cpp stubs/wx/stubWxXhMenu.cpp stubs/wx/stubWxButton.cpp stubs/wx/stubWxFileDlg.cpp stubs/wx/stubWxTopLevel.cpp stubs/wx/stubWxIconBndl.cpp stubs/wx/stubWxIcon.cpp stubs/wx/stubWxToolbar.cpp stubs/wx/stubWxMenu.cpp stubs/wx/stubWxScrolWin.cpp stubs/wx/stubWxPen.cpp stubs/wx/stubWxListCtrl.cpp stubs/wx/stubWxImagPng.cpp stubs/wx/stubWxDcMemory.cpp stubs/wx/stubWxDc.cpp stubs/wx/stubWxDcClient.cpp stubs/wx/stubWxGraphics.cpp stubs/wx/stubWxXhText.cpp stubs/wx/stubWxPanel.cpp stubs/wx/stubWxListBase.cpp stubs/wx/stubWxStatText.cpp stubs/wx/stubWxChoice.cpp stubs/wx/stubWxImagList.cpp stubs/wx/stubWxStatBmp.cpp stubs/wx/stubWxDnd.cpp stubs/wx/stubWxTextDlg.cpp stubs/wx/stubWxDataObj.cpp stubs/wx/stubWxCheckBox.cpp stubs/wx/stubWxStatusBr.cpp stubs/wx/stubWxAuiFrameManager.cpp stubs/wx/stubWxXhPanel.cpp stubs/wx/stubWxXhSizer.cpp stubs/wx/stubWxGdiCmn.cpp stubs/wx/stubWxXhBmpBt.cpp stubs/wx/stubWxXhNotBk.cpp stubs/wx/stubWxSocket.cpp stubs/wx/stubWxSockAddr.cpp stubs/wx/stubWxDirDlg.cpp stubs/wx/stubWxContrainr.cpp stubs/wx/stubWxSizer.cpp stubs/wx/stubWxDisplay.cpp stubs/wx/stubWxUtils.cpp stubs/wx/stubWxSpinCtrl.cpp stubs/wx/stubWxXhScWin.cpp stubs/wx/stubWxSettings.cpp stubs/wx/stubWxXhDlg.cpp stubs/wx/stubWxXhStBmp.cpp stubs/wx/stubWxXhGauge.cpp stubs/wx/stubWxRadioBut.cpp stubs/wx/stubWxNotebook.cpp stubs/wx/stubWxHxTree.cpp stubs/wx/stubWxXhStTxt.cpp stubs/wx/stubWxXhStBox.cpp stubs/wx/stubWxXhStLin.cpp stubs/wx/stubWxXhSpin.cpp stubs/wx/stubWxXhBttn.cpp stubs/wx/stubWxXhListC.cpp stubs/wx/stubWxXhChkb.cpp stubs/wx/stubWxXhChckL.cpp stubs/wx/stubWxXhRadBt.cpp stubs/wx/stubWxBookCtrl.cpp stubs/wx/stubWxXhListB.cpp stubs/wx/stubWxXhHyperlink.cpp stubs/wx/stubWxXhChoic.cpp stubs/wx/stubWxClipbrd.cpp stubs/wx/stubWxValidate.cpp
SQLLITE_STUBS = stubs/sqlite/stubSqlite.cpp
TINYXML_STUBS = stubs/tinyxml/stubTinyXML.cpp
ALL_STUBS = $(WX_STUBS) $(SQLLITE_STUBS) $(TINYXML_STUBS)
MAIN = main/main.cpp

filezilla_engine_tests_CPPFLAGS = $(WX_CPPFLAGS) -I$(srcdir)/../src/engine
filezilla_engine_tests_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_engine_tests_CFLAGS = $(WX_CFLAGS_ONLY)
filezilla_engine_tests_LDFLAGS = ../src/engine/libengine.a -lwx_base_carbonu-2.8
filezilla_engine_tests_SOURCES = $(MAIN) engine/TestEngineDirectoryCache.cpp engine/TestIOThread.cpp engine/TestSocket.cpp $(ALL_STUBS) 

filezilla_interface_tests_CPPFLAGS = $(WX_CPPFLAGS) -I$(srcdir)/../src/interface -I$(srcdir)/../src/engine
filezilla_interface_tests_CXXFLAGS = $(WX_CXXFLAGS_ONLY)
filezilla_interface_tests_CFLAGS = $(WX_CFLAGS_ONLY)
filezilla_interface_tests_LDFLAGS = ../src/interface/libinterface.a -lwx_base_carbonu-2.8
filezilla_interface_tests_SOURCES = $(MAIN) interface/TestAboutDialog.cpp $(ALL_STUBS)



