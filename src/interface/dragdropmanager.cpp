#include "FileZilla.h"
#include "dragdropmanager.h"

CDragDropManager* CDragDropManager::m_pDragDropManager = 0;

CDragDropManager::CDragDropManager()
{
	pDragSource = 0;
}

CDragDropManager* CDragDropManager::Init()
{
	wxASSERT(!m_pDragDropManager);
	m_pDragDropManager = new CDragDropManager;
	return m_pDragDropManager;
}

void CDragDropManager::Release()
{
	delete m_pDragDropManager;
	m_pDragDropManager = 0;
}
