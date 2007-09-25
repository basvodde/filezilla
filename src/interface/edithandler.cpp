#include "FileZilla.h"
#include "edithandler.h"

CEditHandler* CEditHandler::m_pEditHandler = 0;

CEditHandler* CEditHandler::Create()
{
	if (!m_pEditHandler)
		m_pEditHandler = new CEditHandler();

	return m_pEditHandler;
}

CEditHandler* CEditHandler::Get()
{
	return m_pEditHandler;
}

wxString CEditHandler::GetLocalDirectory()
{
	wxString dir = wxFileName::GetTempDir();
	if (dir == _T(""))
		return _T("");

	if (dir.Last() != wxFileName::GetPathSeparator())
		dir += wxFileName::GetPathSeparator();

	// On POSIX, the permissions of the created directory (700) ensure 
	// that this is a safe operation.
	// On Windows, the user's profile directory and associated temp dir
	// already has the correct permissions which get inherited.
	int i = 1;
	do
	{
		wxString newDir = dir + wxString::Format(_T("fz3temp-%d"), i++);
		if (wxFileName::FileExists(newDir) || wxFileName::DirExists(newDir))
			continue;
		
		if (!wxMkdir(newDir, 0700))
			return _T("");

		m_localDir = newDir + wxFileName::GetPathSeparator();
		break;
	} while (true);

	return m_localDir;
}

void CEditHandler::Release()
{
	if (m_localDir != _T(""))
	{
		// TODO: Delete files
		wxRmdir(m_localDir);
	}

	delete this;
}

enum CEditHandler::fileState CEditHandler::GetFileState(const wxString& fileName)
{
	std::list<t_fileData>::const_iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return unknown;
	
	return iter->state;
}

int CEditHandler::GetFileCount() const
{
	return m_fileDataList.size();
}

bool CEditHandler::AddFile(const wxString& fileName)
{
	wxASSERT(GetFileState(fileName) == unknown);
	if (GetFileState(fileName) != unknown)
		return false;

	t_fileData data;
	data.name = fileName;
	data.state = download;
	m_fileDataList.push_back(data);

	return true;
}

bool CEditHandler::Remove(const wxString& fileName)
{
	std::list<t_fileData>::iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return true;

	wxASSERT(iter->state != download && iter->state != upload && iter->state != upload_and_remove);
	if (iter->state == download || iter->state == upload || iter->state == upload_and_remove)
		return false;

	if (wxFileName::FileExists(m_localDir + iter->name))
	{
		if (!wxRemoveFile(m_localDir + iter->name))
		{
			iter->state = removing;
			return false;
		}
	}

	m_fileDataList.erase(iter);

	return true;
}

bool CEditHandler::RemoveAll(const wxString& fileName)
{
	std::list<t_fileData> keep;

	for (std::list<t_fileData>::iterator iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->state == download || iter->state == upload || iter->state == upload_and_remove)
		{
			keep.push_back(*iter);
			continue;
		}
		
		if (wxFileName::FileExists(m_localDir + iter->name))
		{
			if (!wxRemoveFile(m_localDir + iter->name))
			{
				iter->state = removing;
				keep.push_back(*iter);
				continue;
			}
		}
	}

	m_fileDataList = keep;

	return m_fileDataList.empty();
}

std::list<CEditHandler::t_fileData>::iterator CEditHandler::GetFile(const wxString& fileName)
{
	std::list<t_fileData>::iterator iter;
	for (iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

std::list<CEditHandler::t_fileData>::const_iterator CEditHandler::GetFile(const wxString& fileName) const
{
	std::list<t_fileData>::const_iterator iter;
	for (iter = m_fileDataList.begin(); iter != m_fileDataList.end(); iter++)
	{
		if (iter->name == fileName)
			break;
	}

	return iter;
}

void CEditHandler::FinishTransfer(bool successful, const wxString& fileName)
{
	std::list<t_fileData>::iterator iter = GetFile(fileName);
	if (iter == m_fileDataList.end())
		return;

	wxASSERT(iter->state == download || iter->state == upload || iter->state == upload_and_remove);

	switch (iter->state)
	{
	case upload_and_remove:
		if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
			iter->state = removing;
		else
			m_fileDataList.erase(iter);
		break;
	case upload:
		if (wxFileName::FileExists(m_localDir + fileName))
			iter->state = edit;
		else
			m_fileDataList.erase(iter);
		break;
	case download:
		if (wxFileName::FileExists(m_localDir + fileName))
		{
			iter->state = edit;
			if (StartEditing(*iter))
				break;
		}
		if (wxFileName::FileExists(m_localDir + fileName) && !wxRemoveFile(m_localDir + fileName))
			iter->state = removing;
		else
			m_fileDataList.erase(iter);
		break;
	}
}

bool CEditHandler::StartEditing(t_fileData& data)
{
	wxASSERT(data.state == edit);

	wxFileName fn(m_localDir, data.name);
	wxFileType* pType = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
	if (!pType)
		return false;

	wxString cmd;
	if (!pType->GetOpenCommand(&cmd, wxFileType::MessageParameters(m_localDir + data.name)))
		return false;
	if (cmd == _T(""))
		return false;
	
	if (!wxExecute(cmd))
		return false;

	return true;
}
