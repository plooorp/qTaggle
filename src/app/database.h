#pragma once

#include <QObject>
#include <QSharedPointer>
#include "sqlite3.h"

#define db Database::instance()

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
		, msg(m_code_str[Ok])
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
private:
	static QStringList m_code_str;
};

class Record
{
public:
	virtual ~Record() = default;
	virtual void fetch() = 0;
};

class Database final : public QObject
{
	Q_OBJECT

public:
	static Database* instance();
	~Database();
	sqlite3* con();
	DBResult open(const QString& path);
	// clearLastOpened should be false only on application close
	DBResult close(bool clearLastOpened = true);
	bool isOpen();
	DBResult begin();
	DBResult commit();
	DBResult rollback();
	bool inTransaction() const;
	QString path() const;
	QString configPath() const;
	void addRecordToRollbackFetch(const QSharedPointer<Record>& record);

signals:
	void opened(const QString& path);
	void closed();
	void committed();
	void rollbacked();

private:
	Database();
	static Database* m_instance;
	QString m_path;
	sqlite3* m_con;
	bool m_inTransaction;
	QList<QSharedPointer<Record>> m_fetchOnRollback;
	int updateSchema_0to1();
};
