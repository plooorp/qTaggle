#include "database.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>

Database::Database(QObject* parent)
	: QObject(parent)
	, m_con(nullptr)
{
	m_onUpdateTimer = new QTimer(this);
	m_onUpdateTimer->setSingleShot(true);
	m_onUpdateTimer->setInterval(250);
	connect(m_onUpdateTimer, &QTimer::timeout, this, [this]() -> void { emit updated(); });
}

Database::~Database()
{
	close(false);
	s_instance = nullptr;
}

sqlite3* Database::con() const
{
	return m_con;
}

Database* Database::instance()
{
	if (s_instance == nullptr)
		s_instance = new Database();
	return s_instance;
}

DBError Database::open(const QString& path)
{
	if (isOpen())
		close();

	if (int rc = sqlite3_open(path.toUtf8(), &m_con) != SQLITE_OK)
	{
		qCritical().nospace() << "Failed to open database at " << path << ": " << sqlite3_errstr(rc);
		close(); // connection still returned even in the event of error
		return DBError(rc);
	}
	m_path = path;
	sqlite3_exec(m_con, "PRAGMA encoding = 'UTF-8';", 0, 0, 0);
	sqlite3_exec(m_con, "PRAGMA foreign_keys = '1';", 0, 0, 0);
	sqlite3_exec(m_con, "PRAGMA journal_mode = 'WAL';", 0, 0, 0);

	// update database schema if need be
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(m_con, "PRAGMA user_version;", -1, &stmt, nullptr);
	sqlite3_step(stmt);
	int user_version = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	if (user_version > CURRENT_USER_VERSION)
	{
		close();
		return DBError(DBError::UnsupportedVersion, u"Database version is newer than the application supports."_s);
	}

	begin();
	switch (user_version)
	{
	case 0:
		if (int rc = migrate_0_to_1() != SQLITE_OK)
		{
			rollback();
			close();
			return DBError(rc);
		}
	}
	commit();
	const std::string q = "PRAGMA user_version = " + std::to_string(CURRENT_USER_VERSION) + ";";
	sqlite3_exec(m_con, q.c_str(), 0, 0, 0);

	QSettings settings;
	// make this database automatically open on next launch
	settings.setValue("lastOpened", path);
	// add this database to recently opened history
	QStringList recentlyOpened = settings.value("GUI/MainWindow/recentlyOpened", QStringList()).toStringList();
	int i = recentlyOpened.indexOf(path);
	if (i == -1)
	{
		// add new history entry
		recentlyOpened.insert(0, path);
		if (recentlyOpened.size() > MAX_RECENTLY_OPENED_HISTORY_SIZE)
			recentlyOpened.resize(MAX_RECENTLY_OPENED_HISTORY_SIZE);
		settings.setValue("GUI/MainWindow/recentlyOpened", recentlyOpened);
	}
	else if (i > 0)
	{
		// move existing entry to top of list
		recentlyOpened.remove(i);
		recentlyOpened.insert(0, path);
		settings.setValue("GUI/MainWindow/recentlyOpened", recentlyOpened);
	}
	
	emit opened(path);
	sqlite3_update_hook(m_con, [](void* arg, int operation, const char* dbName, const char* tableName, sqlite3_int64 rowid) -> void
		{
			db->m_onUpdateTimer->start();
		}, nullptr);
	//sqlite3_trace_v2(m_con, SQLITE_TRACE_STMT, [](unsigned int mask, void* context, void* p, void* x) -> int
	//	{
	//		const char* sql = static_cast<const char*>(x);
	//		if (sql)
	//			qDebug() << sql;
	//		return 0;
	//	}, nullptr);
	return DBError();
}

DBError Database::close(bool clearLastOpened)
{
	int rc = sqlite3_close(m_con);
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
		return DBError();
	}
	qCritical() << "Failed to close database:" << sqlite3_errstr(rc);
	return DBError(rc);
}

bool Database::isOpen() const
{
	return m_con;
}

bool Database::isClosed() const
{
	return !m_con;
}

DBError Database::begin()
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	return DBError(sqlite3_exec(m_con, "BEGIN TRANSACTION;", 0, 0, 0));
}

DBError Database::commit()
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	int rc = sqlite3_exec(m_con, "COMMIT TRANSACTION;", 0, 0, 0);
	if (rc == SQLITE_OK)
	{
		emit committed();
		return DBError();
	}
	return DBError(rc);
}

DBError Database::rollback()
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	int rc = sqlite3_exec(m_con, "ROLLBACK TRANSACTION;", 0, 0, 0);
	if (rc == SQLITE_OK)
	{
		emit rollbacked();
		return DBError();
	}
	return DBError(rc);
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

int Database::migrate_0_to_1()
{
	const char* sql = R"(
		CREATE TABLE file(
			id       INTEGER PRIMARY KEY AUTOINCREMENT,
			name     TEXT    NOT NULL,
			dir      TEXT    NOT NULL,
			alias    TEXT    NOT NULL DEFAULT '',
			state    INTEGER NOT NULL DEFAULT 0, -- Ok
			comment  TEXT    NOT NULL DEFAULT '',
			source   TEXT    NOT NULL DEFAULT '',
			sha1     BLOB    NOT NULL,
			created  INTEGER NOT NULL DEFAULT (unixepoch()),
			modified INTEGER NOT NULL DEFAULT (unixepoch()),
			checked  INTEGER NOT NULL DEFAULT (unixepoch()),

			UNIQUE (name, dir)
		) STRICT;

		CREATE INDEX file_alias ON file(alias);
		CREATE INDEX file_state ON file(state);
		CREATE INDEX file_created ON file(created);
		CREATE INDEX file_modified ON file(modified);
		CREATE INDEX file_checked ON file(checked);

		CREATE VIRTUAL TABLE file_search USING fts5(name, alias, dir, comment, content='file', content_rowid='id');
		CREATE TRIGGER file_ai AFTER INSERT ON file
		BEGIN
			INSERT INTO file_search(rowid, name, alias, dir, comment)
			VALUES (NEW.id, NEW.name, NEW.alias, NEW.dir, NEW.comment);
		END;
		CREATE TRIGGER file_ad AFTER DELETE ON file
		BEGIN
			INSERT INTO file_search(file_search, rowid, name, alias, dir, comment)
			VALUES ('delete', OLD.id, OLD.name, OLD.alias, OLD.dir, OLD.comment);
		END;
		CREATE TRIGGER file_au AFTER UPDATE ON file
		BEGIN
			INSERT INTO file_search(file_search, rowid, name, alias, dir, comment)
			VALUES ('delete', OLD.id, OLD.name, OLD.alias, OLD.dir, OLD.comment);
			INSERT INTO file_search(rowid, name, alias, dir, comment)
			VALUES (NEW.id, NEW.name, NEW.alias, NEW.dir, NEW.comment);
		END;

		CREATE TABLE tag(
			id          INTEGER PRIMARY KEY AUTOINCREMENT,
			name        TEXT    NOT NULL UNIQUE,
			description TEXT    NOT NULL DEFAULT '',
			degree      INTEGER NOT NULL DEFAULT 0,
			created     INTEGER NOT NULL DEFAULT (unixepoch()),
			modified    INTEGER NOT NULL DEFAULT (unixepoch())
		) STRICT;

		CREATE INDEX tag_name ON tag(name);
		CREATE INDEX tag_degree ON tag(degree);
		CREATE INDEX tag_created ON tag(created);
		CREATE INDEX tag_modified ON tag(modified);

		CREATE VIRTUAL TABLE tag_search USING fts5(name, description, content='tag', content_rowid='id');
		CREATE TRIGGER tag_ai AFTER INSERT ON tag
		BEGIN
			INSERT INTO tag_search(rowid, name, description)
			VALUES (NEW.id, NEW.name, NEW.description);
		END;
		CREATE TRIGGER tag_ad AFTER DELETE ON tag
		BEGIN
			INSERT INTO tag_search(tag_search, rowid, name, description)
			VALUES ('delete', OLD.id, old.name, old.description);
		END;
		CREATE TRIGGER tag_au AFTER UPDATE ON tag
		BEGIN
			INSERT INTO tag_search(tag_search, rowid, name, description)
			VALUES ('delete', OLD.id, OLD.name, OLD.description);
			INSERT INTO tag_search(rowid, name, description)
			VALUES (NEW.id, NEW.name, NEW.description);
		END;

		CREATE TABLE tag_url(
			tag_id INTEGER NOT NULL,
			url    TEXT    NOT NULL,

			PRIMARY KEY (tag_id, url),
			FOREIGN KEY (tag_id) REFERENCES tag(id) ON DELETE CASCADE
		) STRICT;

		CREATE TABLE file_tag(
			file_id INTEGER NOT NULL,
			tag_id  INTEGER NOT NULL,
			created INTEGER NOT NULL DEFAULT (unixepoch()),
			PRIMARY KEY (file_id, tag_id),
			FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
			FOREIGN KEY (tag_id)  REFERENCES tag(id)  ON DELETE CASCADE
		) STRICT;

		CREATE TRIGGER file_tag_ai AFTER INSERT ON file_tag
		BEGIN
			UPDATE tag SET degree = degree + 1 WHERE id = NEW.tag_id;
		END;

		CREATE TRIGGER file_tag_ad AFTER DELETE ON file_tag
		BEGIN
			UPDATE tag SET degree = degree - 1 WHERE id = OLD.tag_id;
		END;

		CREATE TRIGGER file_tag_au AFTER UPDATE ON file_tag
		BEGIN
			UPDATE tag SET degree = degree - 1 WHERE id = OLD.tag_id;
			UPDATE tag SET degree = degree + 1 WHERE id = NEW.tag_id;
		END;
	)";
	int rc = sqlite3_exec(m_con, sql, 0, 0, 0);
	if (rc != SQLITE_OK)
		qCritical() << "Failed updating database from user_version 0 to 1:" << sqlite3_errmsg(m_con);
	return rc;
}

const QStringList DBError::CODE_STRING
{
	u"Ok"_s,
	u""_s, // sqlite3_errstr() is used instead
	u"Database is closed"_s,
	u"Invalid parameter value"_s,
	u"File read/write error"_s,
	u"Unsupported database version"_s
};

Database* Database::s_instance = nullptr;
const int Database::CURRENT_USER_VERSION = 1;
const int Database::MAX_RECENTLY_OPENED_HISTORY_SIZE = 6;
