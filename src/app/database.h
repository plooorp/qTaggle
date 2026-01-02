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
		Ok = 0,
		SQLiteError,
		DatabaseClosed,
		ValueError,
		FileIOError,
		UnsupportedVersion
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

class Database final : public QObject
{
	Q_OBJECT

public:
	static Database* instance();
	~Database() override;
	sqlite3* con() const;
	DBError open(const QString& path);
	/**
	 * @param clearLastOpened When true, prevents the current database from
	 * being automatically opened on next application startup. Should only be
	 * false on application close.
	 */
	DBError close(bool clearLastOpened = true);
	bool isOpen();
	DBError begin();
	DBError commit();
	DBError rollback();
	QString path() const;
	QString configPath() const;

signals:
	void opened(const QString& path);
	void closed();
	void committed();
	void rollbacked();
	// debounced signal from sqlite_update_hook
	void updated();

private:
	Database();
	static Database* m_instance;
	static const int CURRENT_USER_VERSION;
	QTimer* onUpdateTimer;
	QString m_path;
	sqlite3* m_con;
	int migrate_0_to_1();
};
