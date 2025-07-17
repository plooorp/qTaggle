#pragma once

#include <exception>
#include <QObject>
#include <QString>
#include "sqlite3.h"

#define db Database::instance()

class Database final : public QObject
{
	Q_OBJECT

public:
	static Database* instance();
	~Database();
	sqlite3* con();
	bool open(const QString& path);
	// clearLastOpened should be false only on application close
	bool close(bool clearLastOpened = true);
	bool isOpen();
	int begin();
	int rollback();
	int commit();
	QString path();
	QString configPath();

signals:
	void opened(const QString& path);
	void closed();

	void fileCreated();
	void fileDeleted();
	void fileUpdated();

private:
	Database();
	static Database* m_instance;
	QString m_path;
	sqlite3* m_con;
	int updateSchema_0to1();
};

//class SQLiteException final : public std::exception
//{
//public:
//	SQLiteException(int rc)
//		: std::exception()
//		, m_rc(rc)
//	{}
//	const char* what() const noexcept override
//	{
//		return sqlite3_errstr(m_rc);
//	}
//private:
//	int m_rc;
//};

struct DBResult
{
public:
	enum Code
	{
		Ok,
		SQLiteError,
		DatabaseClosed,
		ValueError,
		FileIOError
	};
	DBResult()
		: code(Ok)
		, msg("Ok")
		, sqlite_code(-1)
	{}
	DBResult(Code _code, QString _msg)
		: code(_code)
		, sqlite_code(-1)
		, msg(_msg)
	{}
	DBResult(int _sqlite_code)
		: code(SQLiteError)
		, sqlite_code(_sqlite_code)
		, msg(sqlite3_errstr(_sqlite_code))
	{}
	explicit operator bool() const
	{
		return code != Ok;
	}
	Code code;
	int sqlite_code;
	QString msg;
};
