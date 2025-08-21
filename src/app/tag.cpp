#include "tag.h"

#include <QUrl>
#include "app/globals.h"

Tag::Tag(sqlite3_stmt* stmt)
	: QObject(nullptr)
	, m_id(sqlite3_column_int64(stmt, 0))
	, m_name(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1)))
	, m_description(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2)))
	, m_urls(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3)).split(';'))
	, m_degree(sqlite3_column_int64(stmt, 4))
	, m_created(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 5)))
	, m_modified(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 6)))
{
	qDebug() << "Tag" << m_name << "created";
}

Tag::~Tag()
{
	m_instances.remove(m_id);
	qDebug() << "Tag" << m_name << "deleted";
}

void Tag::fetch()
{
	if (!db->isOpen())
		return;
	sqlite3_stmt* stmt;
	std::string sql = "SELECT * FROM tag WHERE id = " + std::to_string(m_id) + ";";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		m_id = sqlite3_column_int64(stmt, 0);
		m_name = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
		m_description = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2));
		m_urls = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3)).split(';');
		m_degree = sqlite3_column_int64(stmt, 4);
		m_created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 5));
		m_modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 6));
	}
	sqlite3_finalize(stmt);
	emit updated();
}

DBError Tag::create(const QString& name, const QString& description, const QStringList& urls
	, QSharedPointer<Tag>* out)
{
	if (!db->isOpen())
		return DBError(DBError::DatabaseClosed);
	QString name_norm = normalizeName(name);
	if (name_norm.isEmpty())
		return DBError(DBError::ValueError, "Name cannot be empty");
	QString urls_str;
	if (urls.isEmpty())
		urls_str = "";
	else
	{
		QStringList urls_cpy;
		for (QString url : urls)
			urls_cpy.append(QUrl(url, QUrl::TolerantMode).toEncoded());
		urls_str = urls_cpy.join(';');
	}

	db->begin();
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO tag(name, description, urls) VALUES(?, ?, ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	QByteArray name_bytes = name_norm.toUtf8();
	QByteArray description_bytes = description.trimmed().toUtf8();
	QByteArray urls_bytes = urls_str.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, description_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, urls_bytes.constData(), -1, SQLITE_STATIC);
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
		sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?", -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			*out = QSharedPointer<Tag>(new Tag(stmt));
			m_instances.insert((*out)->id(), out->toWeakRef());
		}
		sqlite3_finalize(stmt);
	}
	return DBError();
}

QSharedPointer<Tag> Tag::fromStmt(sqlite3_stmt* stmt)
{
	int64_t id = sqlite3_column_int64(stmt, 0);
	if (m_instances.contains(id))
		if (QSharedPointer<Tag> tag = m_instances.value(id).toStrongRef())
			return tag;
	QSharedPointer<Tag> tag(new Tag(stmt));
	m_instances.insert(tag->id(), tag.toWeakRef());
	return tag;
}

QSharedPointer<Tag> Tag::fromID(const int64_t id)
{
	if (m_instances.contains(id))
		if (QSharedPointer<Tag> tag = m_instances.value(id).toStrongRef())
			return tag;
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	std::string q = "SELECT * FROM tag WHERE id = ?";
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), q.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, id);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		tag = QSharedPointer<Tag>(new Tag(stmt));
		m_instances.insert(tag->id(), tag.toWeakRef());
	}
	else
		qCritical() << "No such tag exists with id" << id;
	sqlite3_finalize(stmt);
	return tag;
}

QSharedPointer<Tag> Tag::fromName(const QString& name)
{
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?;", -1, &stmt, nullptr);
	QByteArray name_bytes = name.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
		tag = fromStmt(stmt);
	sqlite3_finalize(stmt);
	return tag;
}

DBError Tag::remove()
{
	if (!db->isOpen())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "DELETE FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	emit deleted();
	return DBError();
}

DBError Tag::setName(const QString& name)
{
	QString name_norm = normalizeName(name);
	if (name.isEmpty())
		return DBError(DBError::ValueError, "Name cannot be empty");

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET name = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray name_bytes = name_norm.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
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

DBError Tag::setDescription(const QString& description)
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET description = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray description_bytes = description.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, description_bytes.constData(), -1, SQLITE_STATIC);
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

DBError Tag::setURLs(const QStringList& urls)
{
	QString urls_str;
	if (urls.isEmpty())
		urls_str = "";
	else
	{
		QStringList urls_cpy;
		for (QString url_str : urls)
		{
			QUrl url(url_str, QUrl::TolerantMode);
			if (!url.isEmpty())
				urls_cpy.append(url.toEncoded());
		}
		urls_str = urls_cpy.join(';');
	}

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET urls = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray urls_str_bytes = urls_str.toUtf8();
	sqlite3_bind_text(stmt, 1, urls_str_bytes.constData(), -1, SQLITE_STATIC);
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

int64_t Tag::id() const
{
	return m_id;
}

QString Tag::name() const
{
	return m_name;
}

QString Tag::description() const
{
	return m_description;
}

QStringList Tag::urls() const
{
	return m_urls;
}

int64_t Tag::degree() const
{
	return m_degree;
}

QDateTime Tag::created() const
{
	return m_created;
}

QDateTime Tag::modified() const
{
	return m_modified;
}

QString Tag::normalizeName(const QString& name)
{
	return name
		.trimmed()
		.toLower()
		.replace(' ', '_');
}

QMap<int64_t, QWeakPointer<Tag>> Tag::m_instances;
