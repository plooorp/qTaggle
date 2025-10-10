#pragma once

#include <QObject>
#include <QSharedPointer>
#include "sqlite3.h"
#include "app/error.h"
#include "app/globals.h"

#define db Database::instance()

struct DBError : public Error
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
	explicit DBError(const Error* parent = nullptr)
		: Error(u"DatabaseError"_s, Ok, m_code_str[Ok], parent)
		, sqlite_code(-1)
	{}
	explicit DBError(Code code, Error* parent = nullptr)
		: Error(u"DatabaseError"_s, code, m_code_str[code], parent)
		, sqlite_code(-1)
	{}
	explicit DBError(Code code, const QString& message, const Error* parent = nullptr)
		: Error(u"DatabaseError"_s, code, message, parent)
		, sqlite_code(-1)
	{}
	explicit DBError(int sqlite_code, const Error* parent = nullptr)
		: Error(u"DatabaseError"_s, SQLiteError, sqlite3_errstr(sqlite_code), parent)
		, sqlite_code(sqlite_code)
	{}
	int sqlite_code;
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
	sqlite3* con() const;
	DBError open(const QString& path);
	// clearLastOpened should be false only on application close
	DBError close(bool clearLastOpened = true);
	bool isOpen();
	DBError begin();
	DBError commit();
	DBError rollback();
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
