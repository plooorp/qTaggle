#include "file.h"

#include <QCryptographicHash>
#include <QFile>
#include <QApplication>

File::File(sqlite3_stmt* stmt)
	: QObject(nullptr)
{
	initFile(stmt);
	qDebug() << "File" << name() << "created";
}

File::~File()
{
	m_instances.remove(m_id);
	qDebug() << "File" << name() << "deleted";
}

void File::initFile(sqlite3_stmt* stmt)
{
	m_id = sqlite3_column_int64(stmt, 0);
	m_path = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
	m_alias = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 4), sqlite3_column_bytes(stmt, 4));
	m_state = (State)sqlite3_column_int64(stmt, 5);
	m_comment = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 6), sqlite3_column_bytes(stmt, 6));
	m_source = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 7), sqlite3_column_bytes(stmt, 7));
	m_sha1 = QByteArray((const char*)sqlite3_column_blob(stmt, 8), SHA1_DIGEST_SIZE);
	m_created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 9));
	m_modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 10));

	sqlite3_stmt* _stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file_tag WHERE file_id = ?;", -1, &_stmt, nullptr);
	sqlite3_bind_int64(_stmt, 1, m_id);
	m_tags.clear();
	while (sqlite3_step(_stmt) == SQLITE_ROW)
		m_tags.append(FileTag(_stmt));
	sqlite3_finalize(_stmt);
}

void File::fetch()
{
	if (!db->isOpen())
		return;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	if (sqlite3_step(stmt) == SQLITE_ROW)
		initFile(stmt);
	sqlite3_finalize(stmt);
}

QSharedPointer<File> File::create(const QString& path, const QString& alias, const QString& comment, const QString& source)
{
	QSharedPointer<File> file;
	QFileInfo fileInfo(path);

	QByteArray sha1 = sha1Digest(path);
	if (sha1.isNull())
		return file;
	const std::string sql = \
		"INSERT INTO file(path, name, dir, alias, state, comment, source, sha1)"
		"VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
	db->begin();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray path_bytes = fileInfo.absoluteFilePath().toUtf8();
	QByteArray name_bytes = fileInfo.fileName().toUtf8();
	QByteArray dir_bytes = fileInfo.absoluteDir().absolutePath().toUtf8();
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	QByteArray source_bytes = source.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, dir_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 5, (int)State::Ok);
	sqlite3_bind_text(stmt, 6, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 7, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 8, sha1.constData(), SHA1_DIGEST_SIZE, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
	{
		db->rollback();
		qCritical() << "Failed to create file:" << sqlite3_errstr(rc);
		return file;
	}
	db->commit();
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file WHERE path = ?", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		file = QSharedPointer<File>(new File(stmt));
		QWeakPointer<File> file_weak = file.toWeakRef();
		m_instances.insert(file->id(), file_weak);
	}
	sqlite3_finalize(stmt);
	return file;
}

int File::remove(QList<QSharedPointer<File>>& files)
{
	if (!db->isOpen())
		return SQLITE_CANTOPEN;
	int rc = 0;
	db->begin();
	sqlite3_stmt* stmt;
	std::string sql = "DELETE FROM file WHERE id = ?;";
	for (QSharedPointer<File> file : files)
	{
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, file->m_id);
		int rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
		{
			db->rollback();
			qCritical() << "Failed to delete file:" << sqlite3_errstr(rc);
			return rc;
		}
	}
	return rc;
}

QSharedPointer<File> File::fromStmt(sqlite3_stmt* stmt)
{
	int64_t id = sqlite3_column_int64(stmt, 0);
	if (m_instances.contains(id))
		if (QSharedPointer<File> file = m_instances.value(id).toStrongRef())
			return file;
	QSharedPointer<File> file(new File(stmt));
	m_instances.insert(file->id(), file.toWeakRef());
	return file;
}

QList<QSharedPointer<File>> File::fromQuery(const QString& query)
{
	QList<QSharedPointer<File>> files;
	QString query_trimmed = query.trimmed();
	QByteArray query_bytes = query_trimmed.toUtf8();
	sqlite3_stmt* stmt;
	if (query_trimmed.isNull() || query_trimmed.isEmpty())
		sqlite3_prepare_v2(db->con(), "SELECT * FROM file;", -1, &stmt, nullptr);
	else
	{
		std::string sql = R"(
			SELECT * FROM file
			WHERE path LIKE concat('%', ?, '%')
				OR alias LIKE concat('%', ?, '%')
			ORDER BY
				CASE
					WHEN alias LIKE concat(?, '%') THEN 1
					WHEN name LIKE concat(?, '%') THEN 2
					WHEN path LIKE concat(?, '%') THEN 3
					
					WHEN name LIKE concat('%', ?, '%') THEN 4
					WHEN path LIKE concat('%', ?, '%') THEN 5
					ELSE 6
				END;
		)";
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 4, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 5, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 6, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 7, query_bytes.constData(), -1, SQLITE_STATIC);
	}
	while (sqlite3_step(stmt) == SQLITE_ROW)
		files.append(fromStmt(stmt));
	sqlite3_finalize(stmt);
	return files;
}

void File::check()
{
	if (!QFileInfo::exists(m_path))
	{
		setState(State::FileMissing);
		return;
	}
	QByteArray sha1 = sha1Digest(m_path);
	if (sha1.isEmpty())
	{
		setState(State::Error);
		return;
	}
	if (sha1 != m_sha1)
	{
		setState(State::ChecksumChanged);
		return;
	}
	if (m_state != State::Ok)
		setState(Ok);
}

DBResult File::addTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO file_tag(file_id, tag_id) VALUES(?, ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	// ignore pk constraint error if the edge already exists
	if (rc == SQLITE_DONE || rc == SQLITE_CONSTRAINT)
	{
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}
	return DBResult(rc);
}

DBResult File::removeTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	const char* sql = "DELETE FROM file_tag WHERE file_id = ? AND tag_id = ?;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult File::setAlias(const QString& alias)
{
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET alias = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult File::setPath(const QString& path)
{
	QFileInfo file(path);

	sqlite3_stmt* stmt;
	std::string sql = "UPDATE file SET path = ?, name = ?, dir = ? WHERE id = ?;";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray path_bytes = file.absoluteFilePath().toUtf8();
	QByteArray name_bytes = file.fileName().toUtf8();
	QByteArray dir_bytes = file.absoluteDir().absolutePath().toUtf8();
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, dir_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 4, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult File::setState(File::State state)
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET state = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, state);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult File::setComment(const QString& comment)
{
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET comment = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult File::setSource(const QString& source)
{
	QByteArray source_bytes = source.trimmed().toUtf8(); // validation
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET source = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

int64_t File::id() const
{
	return m_id;
}

QString File::name() const
{
	if (m_alias.isEmpty())
		return QFileInfo(m_path).fileName();
	return m_alias;
}

QString File::alias() const
{
	return m_alias;
}

QString File::path() const
{
	return m_path;
}

File::State File::state() const
{
	return m_state;
}

QString File::comment() const
{
	return m_comment;
}

QString File::source() const
{
	return m_source;
}

QByteArray File::sha1() const
{
	return m_sha1;
}

QDateTime File::created() const
{
	return m_created;
}

QDateTime File::modified() const
{
	return m_modified;
}

QList<FileTag> File::tags() const
{
	return m_tags;
}

QString File::stateString(State state)
{
	return m_stateString[state];
}

QByteArray File::sha1Digest(const QString& path)
{
	QCryptographicHash hash(QCryptographicHash::Sha1);
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return QByteArray();
	char buff[4096];
	while (!file.atEnd())
	{
		qint64 bytes = file.read(buff, 4096);
		if (bytes < 0)
			return QByteArray();
		hash.addData(buff, bytes);
	}
	return hash.result();
}

QMap<int64_t, QWeakPointer<File>> File::m_instances;
QStringList File::m_stateString = { "Ok", "Error", "File missing", "Checksum changed"};
