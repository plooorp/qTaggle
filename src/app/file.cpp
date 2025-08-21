#include "file.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include "app/globals.h"

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
	m_alias = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3));
	m_state = (State)sqlite3_column_int64(stmt, 4);
	m_comment = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 5), sqlite3_column_bytes(stmt, 5));
	m_source = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 6), sqlite3_column_bytes(stmt, 6));
	m_sha1 = QByteArray((const char*)sqlite3_column_blob(stmt, 7), SHA1_DIGEST_SIZE_BYTES);
	m_created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 8));
	m_modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 9));
	m_checked = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 10));

	sqlite3_stmt* _stmt;
	const char* sql = R"(
		SELECT ft.* FROM file_tag AS ft
		INNER JOIN tag ON tag.id = ft.tag_id
		WHERE ft.file_id = ?
		ORDER BY tag.name ASC;
	)";
	sqlite3_prepare_v2(db->con(), sql, -1, &_stmt, nullptr);
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

DBError File::create(const QString& path, const QString& alias, const QString& comment, const QString& source, QSharedPointer<File>* out)
{
	if (!db->isOpen())
		return DBError(DBError::DatabaseClosed);
	QFileInfo fileInfo(path);
	QByteArray sha1 = sha1Digest(path);
	if (sha1.isNull())
		return DBError(DBError::FileIOError, "Failed to calculate SHA1 digest");
	const char* sql = R"(
		INSERT INTO file(path, name, alias, state, comment, source, sha1)
		VALUES(?, ?, ?, ?, ?, ?, ?)
	)";
	db->begin();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	QByteArray path_bytes = fileInfo.absoluteFilePath().toUtf8();
	QByteArray name_bytes = fileInfo.fileName().toUtf8();
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	QByteArray source_bytes = source.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 4, Ok);
	sqlite3_bind_text(stmt, 5, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 6, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 7, sha1.constData(), SHA1_DIGEST_SIZE_BYTES, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
	{
		db->rollback();
		return DBError(rc);
	}
	db->commit();
	if (out)
	{
		sqlite3_prepare_v2(db->con(), "SELECT * FROM file WHERE path = ?", -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			*out = QSharedPointer<File>(new File(stmt));
			m_instances.insert((*out)->id(), out->toWeakRef());
		}
		sqlite3_finalize(stmt);
	}
	return DBError();
}

DBError File::remove()
{
	if (!db->isOpen())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "DELETE FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	emit deleted();
	return DBError();
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

CheckError File::check()
{
	CheckError ok = CheckError();
	QByteArray sha1 = sha1Digest(m_path);
	if (!QFileInfo::exists(m_path))
	{
		if (DBError error = setState(FileMissing))
			return CheckError(CheckError::Fail, u"Failed to update file state to FileMissing"_s, &error);
	}
	else if (sha1.isEmpty())
	{
		if (DBError error = setState(Error))
			return CheckError(CheckError::Fail, u"Failed to update state to Error"_s, &error);
	}
	else if (sha1 != m_sha1)
	{
		if (DBError error = setState(ChecksumChanged))
			return CheckError(CheckError::Fail, u"Failed to set state to ChecksumChanged"_s, &error);
		ok.sha1 = sha1;
	}
	else
		if (DBError error = setState(Ok))
			return CheckError(CheckError::Fail, u"Failed to set state back to Ok"_s, &error);
	updateChecked();
	return ok;

}

DBError File::addTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO file_tag(file_id, tag_id) VALUES(?, ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	// ignore pk constraint error if the edge already exists
	if (rc != SQLITE_DONE && rc != SQLITE_CONSTRAINT)
		return DBError(rc);
	if (db->inTransaction())
	{
		db->addRecordToRollbackFetch(m_instances.value(m_id));
		db->addRecordToRollbackFetch(tag);
	}
	fetch();
	tag->fetch();
	return DBError();
}

DBError File::removeTag(const QSharedPointer<Tag>& tag)
{
	sqlite3_stmt* stmt;
	const char* sql = "DELETE FROM file_tag WHERE file_id = ? AND tag_id = ?;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
	{
		db->addRecordToRollbackFetch(m_instances.value(m_id));
		db->addRecordToRollbackFetch(tag);
	}
	fetch();
	tag->fetch();
	return DBError();
}

DBError File::setAlias(const QString& alias)
{
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET alias = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::setPath(const QString& path)
{
	QFileInfo file(path);

	sqlite3_stmt* stmt;
	std::string sql = "UPDATE file SET path = ?, name = ? WHERE id = ?;";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray path_bytes = file.absoluteFilePath().toUtf8();
	QByteArray name_bytes = file.fileName().toUtf8();
	sqlite3_bind_text(stmt, 1, path_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::setState(File::State state)
{
	if (!db->isOpen() || state == m_state)
		return DBError();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET state = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, state);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::setComment(const QString& comment)
{
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET comment = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::setSource(const QString& source)
{
	QByteArray source_bytes = source.trimmed().toUtf8(); // validation
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET source = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::setSHA1(const QByteArray& sha1)
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET sha1 = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_blob(stmt, 1, sha1, SHA1_DIGEST_SIZE_BYTES, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
}

DBError File::updateChecked()
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET checked = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, QDateTime::currentSecsSinceEpoch());
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBError();
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

QDateTime File::checked() const
{
	return m_checked;
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
