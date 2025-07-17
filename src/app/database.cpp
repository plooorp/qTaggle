#include "database.h"

#include <QSettings>
#include <QFileInfo>
#include <QDir>

Database::Database()
	: QObject(nullptr)
	, m_con(nullptr)
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

bool Database::open(const QString& path)
{
	if (isOpen())
	{
		// ignore attemps to open the same database that is already open
		if (path == m_path)
			return false;
		close();
	}

	if (int rc = sqlite3_open(path.toUtf8(), &m_con) != SQLITE_OK)
	{
		qFatal().nospace() << "Failed to open database at " << path << ": " << sqlite3_errstr(rc);
		return false;
	}
	m_path = path;
	sqlite3_stmt* stmt;
	sqlite3_exec(m_con, "PRAGMA encoding = 'UTF-8';", 0, 0, 0);
	sqlite3_exec(m_con, "PRAGMA foreign_keys = '1';", 0, 0, 0);

	// update database schema if need be
	sqlite3_prepare_v2(m_con, "PRAGMA user_version;", -1, &stmt, nullptr);
	sqlite3_step(stmt);
	int user_version = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	begin();
	switch (user_version)
	{
	case 0:
		if (updateSchema_0to1() != SQLITE_OK)
		{
			rollback();
			return false;
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
	return true;
}

bool Database::close(bool clearLastOpened)
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
		return true;
	}
	qCritical() << "Failed to close database:" << sqlite3_errstr(rc);
	return false;
}

bool Database::isOpen()
{
	return (bool)m_con;
}

int Database::begin()
{
	int rc = sqlite3_exec(m_con, "BEGIN TRANSACTION;", 0, 0, 0);
	if (rc != SQLITE_OK)
		qCritical() << sqlite3_errmsg(m_con);
	return rc;
}

int Database::rollback()
{
	int rc = sqlite3_exec(m_con, "ROLLBACK;", 0, 0, 0);
	if (rc != SQLITE_OK)
		qCritical() << sqlite3_errmsg(m_con);
	return rc;
}

QString Database::path()
{
	return m_path;
}

QString Database::configPath()
{
	if (m_path.isNull())
		return QString();
	QFileInfo dbFile(m_path);
	QFileInfo iniFile(dbFile.dir(), dbFile.baseName() + ".ini");
	return iniFile.absoluteFilePath();
}

int Database::commit()
{
	int rc = sqlite3_exec(m_con, "COMMIT;", 0, 0, 0);
	if (rc != SQLITE_OK)
		qCritical() << sqlite3_errmsg(m_con);
	return rc;
}

int Database::updateSchema_0to1()
{
	const std::string sql = R"(
		CREATE TABLE file(
			id          INTEGER PRIMARY KEY AUTOINCREMENT,
			name        TEXT    NOT NULL,
			path        TEXT    NOT NULL UNIQUE,
			dir         TEXT    NOT NULL,
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
		AFTER UPDATE OF name, path, dir, comment, source, sha1digest ON file
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

Database* Database::m_instance = nullptr;
