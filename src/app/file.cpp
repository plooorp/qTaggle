#include "file.h"

#include <QCryptographicHash>
#include <QFile>
#include <QApplication>

File::File(sqlite3_stmt* stmt)
	: m_id(sqlite3_column_int64(stmt, 0))
	, m_name(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1)))
	, m_path(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2)))
	, m_dir(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3)))
	, m_state((State)sqlite3_column_int64(stmt, 4))
	, m_comment(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 5), sqlite3_column_bytes(stmt, 5)))
	, m_sha1(QByteArray((const char*)sqlite3_column_blob(stmt, 7), 20))
	, m_created(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 8)))
	, m_modified(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 9)))
{
	fetchTags();
	qDebug() << "File" << m_name << "created";
}

File::~File()
{
	m_instances.remove(m_id);
	qDebug() << "File" << m_name << "deleted";
}

void File::fetch()
{
	if (!db->isOpen())
		return;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		m_id = sqlite3_column_int64(stmt, 0);
		m_name = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
		m_path = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2));
		m_dir = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3));
		m_state = (State)sqlite3_column_int64(stmt, 4);
		m_comment = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 5), sqlite3_column_bytes(stmt, 5));
		m_sha1 = QByteArray((const char*)sqlite3_column_blob(stmt, 7), 20);
		m_created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 8));
		m_modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 9));
	}
	sqlite3_finalize(stmt);
	fetchTags();
}

void File::fetchTags()
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file_tag WHERE file_id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	m_tags.clear();
	while (sqlite3_step(stmt) == SQLITE_ROW)
		m_tags.append(FileTag(stmt));
	sqlite3_finalize(stmt);
}

QSharedPointer<File> File::create(const QString& name, const QString& path, const QString& comment
	, const QString& source)
{
	QSharedPointer<File> file;
	QFileInfo fileInfo(path);

	QString _name;
	if (name.isNull() || name.isEmpty())
		_name = fileInfo.fileName();
	else
		_name = name.trimmed();

	QByteArray sha1 = sha1Digest(path);
	if (sha1.isNull())
		return file;
	const std::string sql = \
		"INSERT INTO file(name, path, dir, state, comment, source, sha1)"
		"VALUES(?, ?, ?, ?, ?, ?, ?)";
	db->begin();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray name_bytes = _name.toUtf8();
	QByteArray path_bytes = fileInfo.absoluteFilePath().toUtf8();
	QByteArray dir_bytes = fileInfo.absoluteDir().absolutePath().toUtf8();
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	QByteArray source_bytes = source.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, dir_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 4, (int)State::Ok);
	sqlite3_bind_text(stmt, 5, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 6, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 7, sha1.constData(), SHA1_DIGEST_SIZE, SQLITE_STATIC);
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
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM file;", -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
		files.append(fromStmt(stmt));
	sqlite3_finalize(stmt);
	return files;
}

DBResult File::addTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	std::string sql = "INSERT INTO file_tag(file_id, tag_id) VALUES(?, ?);";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
}

DBResult File::removeTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	std::string sql = "DELETE FROM file_tag WHERE file_id = ? AND tag_id = ?;";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
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

int64_t File::id() const
{
	return m_id;
}

QString File::name() const
{
	return m_name;
}

DBResult File::setName(const QString& name)
{
	QByteArray name_bytes = name.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET name = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
}

QString File::path() const
{
	return m_path;
}

DBResult File::setPath(const QString& path)
{
	QFileInfo file(path);
	QByteArray sha1 = sha1Digest(file.absoluteFilePath());
	if (sha1.isEmpty())
		return DBResult(DBResult::FileIOError, "Error opening or reading file");

	sqlite3_stmt* stmt;
	std::string sql = "UPDATE file SET path = ?, dir = ?, sha1 = ? WHERE id = ?;";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray path_bytes = file.absoluteFilePath().toUtf8();
	QByteArray dir_bytes = file.absoluteDir().absolutePath().toUtf8();
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, dir_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 3, sha1.constData(), SHA1_DIGEST_SIZE, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 4, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
}

QString File::dir() const
{
	return m_dir;
}


File::State File::state() const
{
	return m_state;
}

bool File::setState(File::State state)
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET state = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, state);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return true;
	return false;
}

QString File::comment() const
{
	return m_comment;
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
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
}

QString File::source() const
{
	return m_source;
}

DBResult File::setSource(const QString& source)
{
	QByteArray source_bytes = source.trimmed().toUtf8(); // validation
	sqlite3_stmt* stmt;
	sqlite3_prepare(db->con(), "UPDATRE file set source = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
		return DBResult();
	return DBResult(rc);
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

QMap<int64_t, QWeakPointer<File>> File::m_instances;
