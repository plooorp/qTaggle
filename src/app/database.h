#pragma once

#include <QObject>
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
	explicit DBError(Code code = Ok)
		: Error(code, CODE_STRING[code])
		, sqlite_code(-1)
	{}
	explicit DBError(Code code, const QString& message)
		: Error(code, message)
		, sqlite_code(-1)
	{}
	explicit DBError(int sqlite_code)
		: Error(SQLiteError, sqlite3_errstr(sqlite_code))
		, sqlite_code(sqlite_code)
	{}
	QString name() const override
	{
		return u"DatabaseError"_s;
	}
	int sqlite_code;
private:
	const static QStringList CODE_STRING;
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
	bool isOpen() const;
	bool isClosed() const;
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
	explicit Database(QObject* parent = nullptr);
	static Database* s_instance;
	static const int CURRENT_USER_VERSION;
	static const int MAX_RECENTLY_OPENED_HISTORY_SIZE;
	QTimer* m_onUpdateTimer;
	QString m_path;
	sqlite3* m_con;
	int migrate_0_to_1();
};
