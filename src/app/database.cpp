#include "database.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>

Database::Database()
	: QObject(nullptr)
	, m_con(nullptr)
	, m_inTransaction(false)
{}

Database::~Database()
{
	close(false);
	m_instance = nullptr;
}

sqlite3* Database::con()
{
	return m_con;
}

Database* Database::instance()
{
	if (m_instance == nullptr)
		m_instance = new Database();
	return m_instance;
}

DBResult Database::open(const QString& path)
{
	if (isOpen())
		close();

	if (int rc = sqlite3_open(path.toUtf8(), &m_con) != SQLITE_OK)
	{
		qFatal().nospace() << "Failed to open database at " << path << ": " << sqlite3_errstr(rc);
		return DBResult(rc);
	}
	m_path = path;
	sqlite3_exec(m_con, "PRAGMA encoding = 'UTF-8';", 0, 0, 0);
	sqlite3_exec(m_con, "PRAGMA foreign_keys = '1';", 0, 0, 0);

	// update database schema if need be
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(m_con, "PRAGMA user_version;", -1, &stmt, nullptr);
	sqlite3_step(stmt);
	int user_version = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	begin();
	switch (user_version)
	{
	case 0:
		if (int rc = updateSchema_0to1() != SQLITE_OK)
		{
			rollback();
			return DBResult(rc);
		}
		user_version++;
	}
	commit();
	std::string q = "PRAGMA user_version = " + std::to_string(user_version) + ";";
	sqlite3_exec(m_con, q.c_str(), 0, 0, 0);

	QSettings settings;
	// make this database auto-open on application next launch
	settings.setValue("lastOpened", path);
	// add this database to recently opened history
	QStringList recentlyOpened = settings.value("GUI/MainWindow/recentlyOpened", QStringList()).toStringList();
	int i = recentlyOpened.indexOf(path);
	if (i == -1)
	{
		// new history entry
		recentlyOpened.insert(0, path);
		const int MAX_RECENTLY_OPENED_HISTORY_SIZE = 6;
		if (recentlyOpened.size() > MAX_RECENTLY_OPENED_HISTORY_SIZE)
			recentlyOpened.resize(MAX_RECENTLY_OPENED_HISTORY_SIZE);
		settings.setValue("GUI/MainWindow/recentlyOpened", recentlyOpened);
	}
	else if (i > 0)
	{
		// move old history entry to first
		recentlyOpened.remove(i);
		recentlyOpened.insert(0, path);
		settings.setValue("GUI/MainWindow/recentlyOpened", recentlyOpened);
	}
	
	emit opened(path);
	return DBResult();
}

DBResult Database::close(bool clearLastOpened)
{
	int rc = sqlite3_close_v2(m_con);
	if (rc == SQLITE_OK)
	{
		if (clearLastOpened)
		{
			QSettings settings;
			settings.setValue("lastOpened", QString());
		}
		m_con = nullptr;
		m_path = QString();
		emit closed();
		return DBResult();
	}
	qCritical() << "Failed to close database:" << sqlite3_errstr(rc);
	return DBResult(rc);
}

bool Database::isOpen()
{
	return (bool)m_con;
}

bool Database::inTransaction() const
{
	return m_inTransaction;
}

DBResult Database::begin()
{
	int rc = sqlite3_exec(m_con, "BEGIN;", 0, 0, 0);
	if (rc == SQLITE_OK)
	{
		m_inTransaction = true;
		return DBResult();
	}
	qCritical() << sqlite3_errmsg(m_con);
	return DBResult(rc);
}

DBResult Database::commit()
{
	int rc = sqlite3_exec(m_con, "COMMIT;", 0, 0, 0);
	if (rc == SQLITE_OK)
	{
		//for (const QSharedPointer<File>& file : m_filesPendingUpdate)
		//	file->fetch();
		//for (const QSharedPointer<Tag>& tag : m_tagsPendingUpdate)
		//	tag->fetch();
		//m_filesPendingUpdate.clear();
		//m_tagsPendingUpdate.clear();
		m_fetchOnRollback.clear();
		m_inTransaction = false;
		emit committed();
		return DBResult();
	}
	qCritical() << sqlite3_errmsg(m_con);
	return DBResult(rc);
}

DBResult Database::rollback()
{
	int rc = sqlite3_exec(m_con, "ROLLBACK;", 0, 0, 0);
	if (rc == SQLITE_OK)
	{
		for (const QSharedPointer<Record>& record : m_fetchOnRollback)
			record->fetch();
		m_fetchOnRollback.clear();
		m_inTransaction = false;
		emit rollbacked();
		return DBResult();
	}
	qCritical() << sqlite3_errmsg(m_con);
	return DBResult(rc);
}

QString Database::path() const
{
	return m_path;
}

QString Database::configPath() const
{
	if (m_path.isNull())
		return QString();
	QFileInfo dbFile(m_path);
	QFileInfo iniFile(dbFile.dir(), dbFile.baseName() + ".ini");
	return iniFile.absoluteFilePath();
}

int Database::updateSchema_0to1()
{
	const std::string sql = R"(
		CREATE TABLE file(
			id          INTEGER PRIMARY KEY AUTOINCREMENT,
			path        TEXT    NOT NULL UNIQUE,
			dir         TEXT    NOT NULL,
			alias       TEXT    NOT NULL,
			state       INTEGER NOT NULL,
			comment     TEXT    NOT NULL,
			source      TEXT    NOT NULL,
			sha1        BLOB    NOT NULL,
			created     INTEGER NOT NULL DEFAULT (unixepoch()),
			modified    INTEGER NOT NULL DEFAULT (unixepoch())
		) STRICT;

		CREATE INDEX file_path ON file(path);

		CREATE TABLE tag(
			id          INTEGER PRIMARY KEY AUTOINCREMENT,
			name        TEXT    NOT NULL UNIQUE,
			description TEXT    NOT NULL,
			urls        TEXT    NOT NULL,
			degree      INTEGER NOT NULL DEFAULT 0,
			created     INTEGER NOT NULL DEFAULT (unixepoch()),
			modified    INTEGER NOT NULL DEFAULT (unixepoch())
		) STRICT;

		CREATE INDEX tag_name ON tag(name);

		CREATE TABLE file_tag(
			file_id INTEGER NOT NULL,
			tag_id  INTEGER NOT NULL,
			created INTEGER NOT NULL DEFAULT (unixepoch()),
			PRIMARY KEY (file_id, tag_id),
			FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
			FOREIGN KEY (tag_id)  REFERENCES tag(id)  ON DELETE CASCADE
		) STRICT;

		CREATE TRIGGER increment_tag_degree
		AFTER INSERT ON file_tag
		BEGIN
			UPDATE tag
			SET    degree = degree + 1
			WHERE  id = NEW.tag_id;
		END;

		CREATE TRIGGER decrement_tag_degree
		AFTER DELETE ON file_tag
		BEGIN
			UPDATE tag
			SET    degree = degree - 1
			WHERE  id = OLD.tag_id;
		END;

		CREATE TRIGGER update_file_modified
		AFTER UPDATE OF path, dir, alias, comment, source, sha1digest ON file
		BEGIN
			UPDATE file
			SET    modified = unixepoch()
			WHERE  id = file.id;
		END;

		CREATE TRIGGER update_tag_modified
		AFTER UPDATE OF name, description, urls ON tag
		BEGIN
			UPDATE tag
			SET    modified = unixepoch()
			WHERE  id = tag.id;
		END;)";
	int rc = sqlite3_exec(m_con, sql.c_str(), 0, 0, 0);
	if (rc != SQLITE_OK)
		qFatal() << "Failed updating database from user_version 0 to 1:" << sqlite3_errmsg(m_con);
	return rc;
}

void Database::addRecordToRollbackFetch(const QSharedPointer<Record>& record)
{
	if (!m_fetchOnRollback.contains(record))
		m_fetchOnRollback.append(record);
}

QStringList DBResult::m_code_str =
{
	"Ok",
	"", // sqlite3_errstr() is used instead
	"Database is closed",
	"Invalid parameter value",
	"File read/write error"
};
Database* Database::m_instance = nullptr;
