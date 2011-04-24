#include <filezilla.h>
#include "queue_storage.h"
#include "Options.h"
#include "queue.h"

#include <sqlite3.h>
#include <wx/wx.h>
#define INVALID_DATA -1000

enum _column_type
{
	text,
	integer
};

enum _column_flags
{
	not_null,
	autoincrement
};

struct _column
{
	const wxChar* const name;
	_column_type type;
	unsigned int flags;
};

namespace server_table_column_names
{
	enum type
	{
		id,
		host,
		port,
		user,
		password,
		account,
		protocol,
		type,
		logontype,
		timezone_offset,
		transfer_mode,
		max_connections,
		encoding,
		bypass_proxy,
		post_login_commands,
		name
	};
}

_column server_table_columns[] = {
	{ _T("id"), integer, not_null | autoincrement },
	{ _T("host"), text, not_null },
	{ _T("port"), integer, 0 },
	{ _T("user"), text, 0 },
	{ _T("password"), text, 0 },
	{ _T("account"), text, 0 },
	{ _T("protocol"), integer, 0 },
	{ _T("type"), integer, 0 },
	{ _T("logontype"), integer, 0 },
	{ _T("timezone_offset"), integer, 0 },
	{ _T("transfer_mode"), text, 0 },
	{ _T("max_connections"), integer, 0 },
	{ _T("encoding"), text, 0 },
	{ _T("bypass_proxy"), integer, 0 },
	{ _T("post_login_commands"), text, 0 },
	{ _T("name"), text, 0 }
};

namespace file_table_column_names
{
	enum type
	{
		id,
		server,
		local_file,
		local_path,
		remote_file,
		remote_path,
		download,
		size,
		error_count,
		priority,
		ascii_file,
		default_exists_action
	};
}

_column file_table_columns[] = {
	{ _T("id"), integer, not_null | autoincrement },
	{ _T("server"), integer, not_null },
	{ _T("local_file"), text, not_null },
	{ _T("local_path"), text, not_null },
	{ _T("remote_file"), text, not_null },
	{ _T("remote_path"), text, not_null },
	{ _T("download"), integer, not_null },
	{ _T("size"), integer, 0 },
	{ _T("error_count"), integer, 0 },
	{ _T("priority"), integer, 0 },
	{ _T("ascii_file"), integer, 0 },
	{ _T("default_exists_action"), integer, 0 }
};


class CQueueStorage::Impl
{
public:
	Impl()
		: db_()
		, insertServerQuery_()
		, insertFileQuery_()
		, selectServersQuery_()
		, selectFilesQuery_()
	{
	}

	void CreateTables();
	void PrepareStatements();

	sqlite3_stmt* PrepareInsertStatement(const wxString& name, const _column*, unsigned int count);

	bool SaveServer(const CServerItem& item);
	bool SaveFile(wxLongLong server, const CFileItem& item);

	bool Bind(sqlite3_stmt* statement, int index, int value);
	bool Bind(sqlite3_stmt* statement, int index, wxLongLong_t value);
	bool Bind(sqlite3_stmt* statement, int index, const wxString& value);
	bool Bind(sqlite3_stmt* statement, int index, const char* const value);
	bool BindNull(sqlite3_stmt* statement, int index);

	wxString GetColumnText(sqlite3_stmt* statement, int index);
	wxLongLong_t GetColumnInt64(sqlite3_stmt* statement, int index, wxLongLong_t def = 0);
	int GetColumnInt(sqlite3_stmt* statement, int index, int def = 0);

	wxLongLong_t ParseServerFromRow(CServer& server);
	wxLongLong_t ParseFileFromRow(CFileItem** pItem);

	sqlite3* db_;

	sqlite3_stmt* insertServerQuery_;
	sqlite3_stmt* insertFileQuery_;

	sqlite3_stmt* selectServersQuery_;
	sqlite3_stmt* selectFilesQuery_;

#ifndef __WXMSW__
	wxMBConvUTF16 utf16_;
#endif
};

void CQueueStorage::Impl::CreateTables()
{
	if (!db_)
		return;

	{
		wxString query( _T("CREATE TABLE IF NOT EXISTS servers (") );

		for (unsigned int i = 0; i < (sizeof(server_table_columns) / sizeof(_column)); ++i)
		{
			if (i)
				query += _T(", ");
			query += server_table_columns[i].name;
			if (server_table_columns[i].type == integer)
				query += _T(" INTEGER");
			else
				query += _T(" TEXT");

			if (server_table_columns[i].flags & autoincrement)
				query += _T(" PRIMARY KEY AUTOINCREMENT");
			if (server_table_columns[i].flags & not_null)
				query += _T(" NOT NULL");
		}
		query += _T(")");

		if (sqlite3_exec(db_, query.ToUTF8(), 0, 0, 0) != SQLITE_OK)
		{
		}
	}
	{
		wxString query( _T("CREATE TABLE IF NOT EXISTS files (") );

		for (unsigned int i = 0; i < (sizeof(file_table_columns) / sizeof(_column)); ++i)
		{
			if (i)
				query += _T(", ");
			query += file_table_columns[i].name;
			if (file_table_columns[i].type == integer)
				query += _T(" INTEGER");
			else
				query += _T(" TEXT");

			if (file_table_columns[i].flags & autoincrement)
				query += _T(" PRIMARY KEY AUTOINCREMENT");
			if (file_table_columns[i].flags & not_null)
				query += _T(" NOT NULL");
		}
		query += _T(")");

		if (sqlite3_exec(db_, query.ToUTF8(), 0, 0, 0) != SQLITE_OK)
		{
		}

		
		query = _T("CREATE INDEX IF NOT EXISTS server_index ON files (server)");
		if (sqlite3_exec(db_, query.ToUTF8(), 0, 0, 0) != SQLITE_OK)
		{
		}
	}
}

sqlite3_stmt* CQueueStorage::Impl::PrepareInsertStatement(const wxString& name, const _column* columns, unsigned int count)
{
	if (!db_)
		return 0;

	wxString query = _T("INSERT INTO ") + name + _T(" (");
	for (unsigned int i = 1; i < count; ++i)
	{
		if (i > 1)
			query += _T(", ");
		query += columns[i].name;
	}
	query += _T(") VALUES (");
	for (unsigned int i = 1; i < count; ++i)
	{
		if (i > 1)
			query += _T(",");
		query += wxString(_T(":")) + columns[i].name;
	}

	query += _T(")");

	sqlite3_stmt* ret = 0;
	if (sqlite3_prepare_v2(db_, query.ToUTF8(), -1, &ret, 0) != SQLITE_OK)
		ret = 0;

	return ret;
}

void CQueueStorage::Impl::PrepareStatements()
{
	insertServerQuery_ = PrepareInsertStatement(_T("servers"), server_table_columns, sizeof(server_table_columns) / sizeof(_column));
	insertFileQuery_ = PrepareInsertStatement(_T("files"), file_table_columns, sizeof(file_table_columns) / sizeof(_column));

	if (db_)
	{
		wxString query = _T("SELECT ");
		for (unsigned int i = 0; i < (sizeof(server_table_columns) / sizeof(_column)); ++i)
		{
			if (i > 0)
				query += _T(", ");
			query += server_table_columns[i].name;
		}

		query += _T(" FROM servers ORDER BY id ASC");

		if (sqlite3_prepare_v2(db_, query.ToUTF8(), -1, &selectServersQuery_, 0) != SQLITE_OK)
			selectServersQuery_ = 0;
	}

	if (db_)
	{
		wxString query = _T("SELECT ");
		for (unsigned int i = 0; i < (sizeof(file_table_columns) / sizeof(_column)); ++i)
		{
			if (i > 0)
				query += _T(", ");
			query += file_table_columns[i].name;
		}

		query += _T(" FROM files WHERE server=:server ORDER BY id ASC");

		if (sqlite3_prepare_v2(db_, query.ToUTF8(), -1, &selectFilesQuery_, 0) != SQLITE_OK)
			selectFilesQuery_ = 0;
	}
}


bool CQueueStorage::Impl::Bind(sqlite3_stmt* statement, int index, int value)
{
	return sqlite3_bind_int(statement, index, value) == SQLITE_OK;
}


bool CQueueStorage::Impl::Bind(sqlite3_stmt* statement, int index, wxLongLong_t value)
{
	return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}


extern "C" {
static void custom_free(void* v)
{
#ifdef __WXMSW__
	wxStringData* data = reinterpret_cast<wxStringData*>(v) - 1;
	data->Unlock();
#else
	char* s = reinterpret_cast<char*>(v);
	delete [] s;
#endif
}
}

bool CQueueStorage::Impl::Bind(sqlite3_stmt* statement, int index, const wxString& value)
{
#ifdef __WXMSW__
	// Increase string reference and pass the data to sqlite with a custom deallocator that
	// reduces the reference once sqlite is done with it.
	wxStringData* data = reinterpret_cast<wxStringData*>(const_cast<wxChar*>(value.c_str())) - 1;
	data->Lock();
	return sqlite3_bind_text16(statement, index, data + 1, data->nDataLength * 2, custom_free) == SQLITE_OK;
#else
	char* out = new char[value.size() * 2];
	size_t outlen = utf16_.FromWChar(out, value.size() * 2, value.c_str(), value.size());
	bool ret = sqlite3_bind_text16(statement, index, out, outlen, custom_free) == SQLITE_OK;
	return ret;
#endif
}


bool CQueueStorage::Impl::Bind(sqlite3_stmt* statement, int index, const char* const value)
{
	return sqlite3_bind_text(statement, index, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}


bool CQueueStorage::Impl::BindNull(sqlite3_stmt* statement, int index)
{
	return sqlite3_bind_null(statement, index) == SQLITE_OK;
}


bool CQueueStorage::Impl::SaveServer(const CServerItem& item)
{
	bool kiosk_mode = COptions::Get()->GetOptionVal(OPTION_DEFAULT_KIOSKMODE) != 0;
	
	const CServer& server = item.GetServer();
	sqlite3_reset(insertServerQuery_);
	
	Bind(insertServerQuery_, server_table_column_names::host, server.GetHost());
	Bind(insertServerQuery_, server_table_column_names::port, static_cast<int>(server.GetPort()));
	Bind(insertServerQuery_, server_table_column_names::protocol, static_cast<int>(server.GetProtocol()));
	Bind(insertServerQuery_, server_table_column_names::type, static_cast<int>(server.GetType()));

	enum LogonType logonType = server.GetLogonType();
	if (server.GetLogonType() != ANONYMOUS)
	{
		Bind(insertServerQuery_, server_table_column_names::user, server.GetUser());
		
		if (server.GetLogonType() == NORMAL || server.GetLogonType() == ACCOUNT)
		{
			if (kiosk_mode)
			{
				logonType = ASK;
				BindNull(insertServerQuery_, server_table_column_names::password);
				BindNull(insertServerQuery_, server_table_column_names::account);
			}
			else
			{
				Bind(insertServerQuery_, server_table_column_names::password, server.GetPass());

				if (server.GetLogonType() == ACCOUNT)
					Bind(insertServerQuery_, server_table_column_names::account, server.GetAccount());
				else
					BindNull(insertServerQuery_, server_table_column_names::account);
			}
		}
		else
		{
			BindNull(insertServerQuery_, server_table_column_names::password);
			BindNull(insertServerQuery_, server_table_column_names::account);
		}
	}
	else
	{
			BindNull(insertServerQuery_, server_table_column_names::user);
			BindNull(insertServerQuery_, server_table_column_names::password);
			BindNull(insertServerQuery_, server_table_column_names::account);
	}
	Bind(insertServerQuery_, server_table_column_names::logontype, static_cast<int>(logonType));

	Bind(insertServerQuery_, server_table_column_names::timezone_offset, server.GetTimezoneOffset());

	switch (server.GetPasvMode())
	{
	case MODE_PASSIVE:
		Bind(insertServerQuery_, server_table_column_names::transfer_mode, _T("passive"));
		break;
	case MODE_ACTIVE:
		Bind(insertServerQuery_, server_table_column_names::transfer_mode, _T("active"));
		break;
	default:
		Bind(insertServerQuery_, server_table_column_names::transfer_mode, _T("default"));
		break;
	}
	Bind(insertServerQuery_, server_table_column_names::max_connections, server.MaximumMultipleConnections());

	switch (server.GetEncodingType())
	{
	default:
	case ENCODING_AUTO:
		Bind(insertServerQuery_, server_table_column_names::encoding, _T("Auto"));
		break;
	case ENCODING_UTF8:
		Bind(insertServerQuery_, server_table_column_names::encoding, _T("UTF-8"));
		break;
	case ENCODING_CUSTOM:
		Bind(insertServerQuery_, server_table_column_names::encoding, server.GetCustomEncoding());
		break;
	}

	const enum ServerProtocol protocol = server.GetProtocol();
	if (protocol == FTP || protocol == FTPS || protocol == FTPES)
	{
		const std::vector<wxString>& postLoginCommands = server.GetPostLoginCommands();
		if (!postLoginCommands.empty())
		{
			wxString commands;
			for (std::vector<wxString>::const_iterator iter = postLoginCommands.begin(); iter != postLoginCommands.end(); iter++)
			{
				if (!commands.empty())
					commands += _T("\n");
				commands += *iter;
			}
			Bind(insertServerQuery_, server_table_column_names::post_login_commands, commands);
		}
		else
			BindNull(insertServerQuery_, server_table_column_names::post_login_commands);
	}
	else
		BindNull(insertServerQuery_, server_table_column_names::post_login_commands);

	Bind(insertServerQuery_, server_table_column_names::bypass_proxy, server.GetBypassProxy() ? 1 : 0);
	if (!server.GetName().empty())
		Bind(insertServerQuery_, server_table_column_names::name, server.GetName());
	else
		BindNull(insertServerQuery_, server_table_column_names::name);

	int res;
	do {
		res = sqlite3_step(insertServerQuery_);
	} while (res == SQLITE_BUSY);
	
	if (res == SQLITE_DONE)
	{
		sqlite3_int64 serverId = sqlite3_last_insert_rowid(db_);
		Bind(insertFileQuery_, file_table_column_names::server, serverId);

		const std::vector<CQueueItem*>& children = item.GetChildren();
		for (std::vector<CQueueItem*>::const_iterator it = children.begin() + item.GetRemovedAtFront(); it != children.end(); ++it)
		{
			CQueueItem* item = *it;
			if (item->GetType() == QueueItemType_File)
				SaveFile(serverId, *reinterpret_cast<CFileItem*>(item));
		}
	}
	return res == SQLITE_DONE;
}


bool CQueueStorage::Impl::SaveFile(wxLongLong server, const CFileItem& file)
{
	if (file.m_edit != CEditHandler::none)
		return true;

	sqlite3_reset(insertFileQuery_);

	Bind(insertFileQuery_, file_table_column_names::local_file, file.GetLocalFile());
	Bind(insertFileQuery_, file_table_column_names::local_path, file.GetLocalPath().GetPath());
	Bind(insertFileQuery_, file_table_column_names::remote_file, file.GetRemoteFile());
	Bind(insertFileQuery_, file_table_column_names::remote_path, file.GetRemotePath().GetSafePath());
	Bind(insertFileQuery_, file_table_column_names::download, file.Download() ? 1 : 0);
	if (file.GetSize() != -1)
		Bind(insertFileQuery_, file_table_column_names::size, file.GetSize().GetValue());
	else
		BindNull(insertFileQuery_, file_table_column_names::size);
	if (file.m_errorCount)
		Bind(insertFileQuery_, file_table_column_names::error_count, file.m_errorCount);
	else
		BindNull(insertFileQuery_, file_table_column_names::error_count);
	Bind(insertFileQuery_, file_table_column_names::priority, file.GetPriority());
	Bind(insertFileQuery_, file_table_column_names::ascii_file, file.m_transferSettings.binary ? 0 : 1);

	if (file.m_defaultFileExistsAction != CFileExistsNotification::unknown)
		Bind(insertFileQuery_, file_table_column_names::default_exists_action, file.m_defaultFileExistsAction);
	else
		BindNull(insertFileQuery_, file_table_column_names::default_exists_action);

	int res;
	do {
		res = sqlite3_step(insertFileQuery_);
	} while (res == SQLITE_BUSY);

	return res == SQLITE_DONE;
}

wxString CQueueStorage::Impl::GetColumnText(sqlite3_stmt* statement, int index)
{
	wxString ret;

#ifdef __WXMSW__
	const wxChar* text = reinterpret_cast<const wxChar*>(sqlite3_column_text16(statement, index));
	if (text)
		ret = text;
#else
	const char* text = reinterpret_cast<const char*>(sqlite3_column_text16(statement, index));
	int len = sqlite3_column_bytes16(statement, index);
	if (text)
	{
		wxChar* out = ret.GetWriteBuf( len );
		int outlen = utf16_.ToWChar( out, len, text, len );
		ret.UngetWriteBuf( outlen );
		ret.Shrink();
	}
#endif

	return ret;
}

wxLongLong_t CQueueStorage::Impl::GetColumnInt64(sqlite3_stmt* statement, int index, wxLongLong_t def)
{
	if (sqlite3_column_type(statement, index) == SQLITE_NULL)
		return def;
	else
		return sqlite3_column_int64(statement, index);
}

int CQueueStorage::Impl::GetColumnInt(sqlite3_stmt* statement, int index, int def)
{
	if (sqlite3_column_type(statement, index) == SQLITE_NULL)
		return def;
	else
		return sqlite3_column_int(statement, index);
}

wxLongLong_t CQueueStorage::Impl::ParseServerFromRow(CServer& server)
{
	server = CServer();

	wxString host = GetColumnText(selectServersQuery_, server_table_column_names::host);
	if (host == _T(""))
		return INVALID_DATA;

	int port = GetColumnInt(selectServersQuery_, server_table_column_names::port);
	if (port < 1 || port > 65535)
		return INVALID_DATA;

	if (!server.SetHost(host, port))
		return INVALID_DATA;

	int protocol = GetColumnInt(selectServersQuery_, server_table_column_names::protocol);
	if (protocol < 0 || protocol > MAX_VALUE)
		return INVALID_DATA;

	server.SetProtocol((enum ServerProtocol)protocol);

	int type = GetColumnInt(selectServersQuery_, server_table_column_names::type);
	if (type < 0 || type >= SERVERTYPE_MAX)
		return INVALID_DATA;

	server.SetType((enum ServerType)type);

	int logonType = GetColumnInt(selectServersQuery_, server_table_column_names::logontype);
	if (logonType < 0 || logonType >= LOGONTYPE_MAX)
		return INVALID_DATA;

	server.SetLogonType((enum LogonType)logonType);

	if (server.GetLogonType() != ANONYMOUS)
	{
		wxString user = GetColumnText(selectServersQuery_, server_table_column_names::user);

		wxString pass;
		if ((long)NORMAL == logonType || (long)ACCOUNT == logonType)
			pass = GetColumnText(selectServersQuery_, server_table_column_names::password);

		if (!server.SetUser(user, pass))
			return INVALID_DATA;

		if ((long)ACCOUNT == logonType)
		{
			wxString account = GetColumnText(selectServersQuery_, server_table_column_names::account);
			if (account == _T(""))
				return INVALID_DATA;
			if (!server.SetAccount(account))
				return INVALID_DATA;
		}
	}

	int timezoneOffset = GetColumnInt(selectServersQuery_, server_table_column_names::timezone_offset);
	if (!server.SetTimezoneOffset(timezoneOffset))
		return INVALID_DATA;

	wxString pasvMode = GetColumnText(selectServersQuery_, server_table_column_names::transfer_mode);
	if (pasvMode == _T("passive"))
		server.SetPasvMode(MODE_PASSIVE);
	else if (pasvMode == _T("active"))
		server.SetPasvMode(MODE_ACTIVE);
	else
		server.SetPasvMode(MODE_DEFAULT);

	int maximumMultipleConnections = GetColumnInt(selectServersQuery_, server_table_column_names::max_connections);
	if (maximumMultipleConnections < 0)
		return INVALID_DATA;
	server.MaximumMultipleConnections(maximumMultipleConnections);

	wxString encodingType = GetColumnText(selectServersQuery_, server_table_column_names::encoding);
	if (encodingType.empty() || encodingType == _T("Auto"))
		server.SetEncodingType(ENCODING_AUTO);
	else if (encodingType == _T("UTF-8"))
		server.SetEncodingType(ENCODING_UTF8);
	else
	{
		if (!server.SetEncodingType(ENCODING_CUSTOM, encodingType))
			return INVALID_DATA;
	}

	if (protocol == FTP || protocol == FTPS || protocol == FTPES)
	{
		std::vector<wxString> postLoginCommands;

		wxString commands = GetColumnText(selectServersQuery_, server_table_column_names::post_login_commands);
		while (!commands.empty())
		{
			int pos = commands.Find('\n');
			if (!pos)
				commands = commands.Mid(1);
			else if (pos == -1)
			{
				postLoginCommands.push_back(commands);
				commands.clear();
			}
			else
			{
				postLoginCommands.push_back(commands.Left(pos));
				commands = commands.Mid(pos + 1);
			}
		}
		if (!server.SetPostLoginCommands(postLoginCommands))
			return INVALID_DATA;
	}
	

	server.SetBypassProxy(GetColumnInt(selectServersQuery_, server_table_column_names::bypass_proxy) == 1 );
	server.SetName( GetColumnText(selectServersQuery_, server_table_column_names::name) );

	return GetColumnInt64(selectServersQuery_, server_table_column_names::id);
}


wxLongLong_t CQueueStorage::Impl::ParseFileFromRow(CFileItem** pItem)
{
	wxString localFile = GetColumnText(selectFilesQuery_, file_table_column_names::local_file);
	wxString safeLocalPath = GetColumnText(selectFilesQuery_, file_table_column_names::local_path);
	wxString remoteFile = GetColumnText(selectFilesQuery_, file_table_column_names::remote_file);
	wxString safeRemotePath = GetColumnText(selectFilesQuery_, file_table_column_names::remote_path);
	bool download = GetColumnInt(selectFilesQuery_, file_table_column_names::download) != 0;

	wxLongLong size = GetColumnInt64(selectFilesQuery_, file_table_column_names::size);
	int errorCount = GetColumnInt(selectFilesQuery_, file_table_column_names::error_count);
	int priority = GetColumnInt(selectFilesQuery_, file_table_column_names::priority, priority_normal);

	bool binary = GetColumnInt(selectFilesQuery_, file_table_column_names::ascii_file) == 0;
	int overwrite_action = GetColumnInt(selectFilesQuery_, file_table_column_names::default_exists_action, CFileExistsNotification::unknown);

	CLocalPath localPath;
	CServerPath remotePath;
	if (localFile != _T("") && localPath.SetPath(safeLocalPath) &&
		remoteFile != _T("") && remotePath.SetSafePath(safeRemotePath) &&
		size >= -1 &&
		priority > 0 && priority < PRIORITY_COUNT &&
		errorCount >= 0)
	{
		// TODO: Coalesce strings
		// CServerPath and wxString are reference counted.
		// Save some memory here by re-using the old copy
		//if (localPath != previousLocalPath)
		//	previousLocalPath = localPath;
		//if (previousRemotePath != remotePath)
		//	previousRemotePath = remotePath;

		CFileItem* fileItem = new CFileItem(0, true, download, localPath, localFile, remoteFile, remotePath, size);
		*pItem = fileItem;
		fileItem->m_transferSettings.binary = binary;
		fileItem->SetPriorityRaw((enum QueuePriority)priority);
		fileItem->m_errorCount = errorCount;

		if (overwrite_action > 0 && overwrite_action < CFileExistsNotification::ACTION_COUNT)
			fileItem->m_defaultFileExistsAction = (CFileExistsNotification::OverwriteAction)overwrite_action;

		return GetColumnInt64(selectFilesQuery_, file_table_column_names::id); 
	}

	return INVALID_DATA;
}

CQueueStorage::CQueueStorage()
: d_(new Impl)
{
	int ret = sqlite3_open(GetDatabaseFilename().ToUTF8(), &d_->db_);
	if (ret != SQLITE_OK)
		d_->db_ = 0;

	if (sqlite3_exec(d_->db_, "PRAGMA encoding=\"UTF-16le\"", 0, 0, 0) == SQLITE_OK)
	{
		d_->CreateTables();
		d_->PrepareStatements();
	}
}

CQueueStorage::~CQueueStorage()
{
	sqlite3_finalize(d_->insertServerQuery_);
	sqlite3_finalize(d_->insertFileQuery_);
	sqlite3_finalize(d_->selectServersQuery_);
	sqlite3_finalize(d_->selectFilesQuery_);
	sqlite3_close(d_->db_);
	delete d_;
}

bool CQueueStorage::SaveQueue(std::vector<CServerItem*> const& queue)
{
	bool ret = false;
	if (sqlite3_exec(d_->db_, "BEGIN TRANSACTION", 0, 0, 0) == SQLITE_OK)
	{
		for (std::vector<CServerItem*>::const_iterator it = queue.begin(); it != queue.end(); ++it)	
			d_->SaveServer(**it);

		ret = sqlite3_exec(d_->db_, "END TRANSACTION", 0, 0, 0) == SQLITE_OK;
	}

	return ret;
}

wxLongLong_t CQueueStorage::GetServer(CServer& server, bool fromBeginning)
{
	wxLongLong_t ret = -1;

	if (d_->selectServersQuery_)
	{
		if (fromBeginning)
			sqlite3_reset(d_->selectServersQuery_);

		while (true)
		{
			int res;
			do
			{
				res = sqlite3_step(d_->selectServersQuery_);
			}
			while (res == SQLITE_BUSY);
			
			if (res == SQLITE_ROW)
			{
				ret = d_->ParseServerFromRow(server);
				if (ret > 0)
					break;
			}
			else if (res == SQLITE_DONE)
			{
				ret = 0;
				break;
			}
		}
	}
	else {
		ret = -1;
	}

	return ret;
}


wxLongLong_t CQueueStorage::GetFile(CFileItem** pItem, wxLongLong_t server)
{
	wxLongLong_t ret = -1;
	*pItem = 0;

	if (d_->selectFilesQuery_)
	{
		if (server > 0)
		{
			sqlite3_reset(d_->selectFilesQuery_);
			sqlite3_bind_int64(d_->selectFilesQuery_, 1, server);
		}

		while (true)
		{
			int res;
			do
			{
				res = sqlite3_step(d_->selectFilesQuery_);
			}
			while (res == SQLITE_BUSY);
			
			if (res == SQLITE_ROW)
			{
				ret = d_->ParseFileFromRow(pItem);
				if (ret > 0)
					break;
			}
			else if (res == SQLITE_DONE)
			{
				ret = 0;
				break;
			}
		}
	}
	else {
		ret = -1;
	}

	return ret;
}

bool CQueueStorage::Clear()
{
	if (!d_->db_)
		return false;

	if (sqlite3_exec(d_->db_, "DELETE FROM files", 0, 0, 0) != SQLITE_OK)
		return false;

	if (sqlite3_exec(d_->db_, "DELETE FROM servers", 0, 0, 0) != SQLITE_OK)
		return false;

	return true;
}

wxString CQueueStorage::GetDatabaseFilename()
{
	wxFileName file(COptions::Get()->GetOption(OPTION_DEFAULT_SETTINGSDIR), _T("queue.sqlite3"));

	return file.GetFullPath();
}

bool CQueueStorage::BeginTransaction()
{
	return sqlite3_exec(d_->db_, "BEGIN TRANSACTION", 0, 0, 0) == SQLITE_OK;
}

bool CQueueStorage::EndTransaction()
{
	return sqlite3_exec(d_->db_, "END TRANSACTION", 0, 0, 0) == SQLITE_OK;
}
