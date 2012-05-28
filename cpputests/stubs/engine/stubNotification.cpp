#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "filezilla.h"
#include "notification.h"

DEFINE_EVENT_TYPE(fzEVT_NOTIFICATION);

CNotification::CNotification() {
	FAIL("CNotification::CNotification");
}

CNotification::~CNotification() {
	FAIL("CNotification::~CNotification");
}

CCertificate::CCertificate(const CCertificate& op) {
	FAIL("CCertificate::CCertificate");
}

CCertificate::~CCertificate() {
	FAIL("CCertificate::~CCertificate");
}

CCertificate& CCertificate::operator=(const CCertificate &op) {
	FAIL("CCertificate::operator=");
	return *this;
}

CTransferStatusNotification::CTransferStatusNotification(
		CTransferStatus *pStatus) {
	FAIL("CTransferStatusNotification:CTransferStatusNotification");
}

CTransferStatusNotification::~CTransferStatusNotification() {
	FAIL("CTransferStatusNotification::~CTransferStatusNotification");
}

enum NotificationId CTransferStatusNotification::GetID() const {
	FAIL("CTransferStatusNotification::GetID");
	return nId_active;
}

const CTransferStatus *CTransferStatusNotification::GetStatus() const {
	FAIL("CTransferStatusNotification::GetStatus");
	return NULL;
}

CHostKeyNotification::CHostKeyNotification(wxString host, int port,
		wxString fingerprint, bool changed) :
	m_host(L""), m_port(0), m_fingerprint(L""), m_changed(false) {
	FAIL("CHostKeyNotification::CHostKeyNotification");
}

CHostKeyNotification::~CHostKeyNotification() {
	FAIL("CHostKeyNotification::~CHostKeyNotification");
}

enum RequestId CHostKeyNotification::GetRequestID() const {
	FAIL("CHostKeyNotification::GetRequestID");
	return reqId_fileexists;
}

wxString CHostKeyNotification::GetHost() const {
	FAIL("CHostKeyNotification::GetHost");
	return L"";
}

int CHostKeyNotification::GetPort() const {
	FAIL("CHostKeyNotification::GetPort");
	return 0;
}

wxString CHostKeyNotification::GetFingerprint() const {
	FAIL("CHostKeyNotification::GetFingerprint");
	return L"";
}

CAsyncRequestNotification::CAsyncRequestNotification() {
	FAIL("CAsyncRequestNotification::CAsyncRequestNotification");
}

CAsyncRequestNotification::~CAsyncRequestNotification() {
	FAIL("CAsyncRequestNotification::~CAsyncRequestNotification");
}

enum NotificationId CAsyncRequestNotification::GetID() const {
	FAIL("CAsyncRequestNotification::GetID");
	return nId_active;
}

CCertificateNotification::CCertificateNotification(const wxString& host,
		unsigned int port, const wxString& sessionCipher,
		const wxString& sessionMac,
		const std::vector<CCertificate> &certificates) {
	FAIL("CCertificateNotification::CCertificateNotification");
}

CCertificateNotification::~CCertificateNotification() {
	FAIL("CCertificateNotification::~CCertificateNotification");
}

CDataNotification::CDataNotification(char* pData, int len) {
	FAIL("CDataNotification::CDataNotification");
}

CDataNotification::~CDataNotification() {
	FAIL("CDataNotification::~CDataNotification");
}

char* CDataNotification::Detach(int& len) {
	FAIL("CDataNotification::Detach");
	return NULL;
}

