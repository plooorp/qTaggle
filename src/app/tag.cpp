#include "tag.h"
#include <QDebug>
#include <QUrl>

Tag::Tag(sqlite3_stmt* stmt)
	: m_id(sqlite3_column_int64(stmt, 0))
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
}

QSharedPointer<Tag> Tag::create(const QString& name, const QString& description, const QStringList& urls)
{
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	QString name_norm = normalizeName(name);
	QString description_norm = description.trimmed();
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

	const std::string q = "INSERT INTO tag(name, description, urls) VALUES(?, ?, ?);";
	db->begin();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), q.c_str(), -1, &stmt, nullptr);
	QByteArray name_ba = name_norm.toUtf8();
	QByteArray desc_ba = description_norm.toUtf8();
	QByteArray urls_ba = urls_str.toUtf8();
	sqlite3_bind_text(stmt, 1, name_ba.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, desc_ba.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, urls_ba.constData(), -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
	{
		db->rollback();
		qCritical() << "Failed to create tag:" << sqlite3_errstr(rc);
		return tag;
	}
	db->commit();
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, name_ba.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		tag = QSharedPointer<Tag>(new Tag(stmt));
		m_instances.insert(tag->id(), tag.toWeakRef());
	}
	sqlite3_finalize(stmt);
	return tag;
}

QSharedPointer<Tag> Tag::create(const QString& name, const QString& description)
{
	return Tag::create(name, description, QStringList());
}

QSharedPointer<Tag> Tag::create(const QString& name)
{
	return Tag::create(name, QString(), QStringList());
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

QList<QSharedPointer<Tag>> Tag::fromQuery(const QString& query)
{
	QList<QSharedPointer<Tag>> tags;
	if (!db->isOpen())
		return tags;
	QString q = query.trimmed();
	std::string sql;
	if (q.isNull() || q.isEmpty())
		sql = "SELECT * FROM tag;";
	else
	{
		sql = "SELECT * FROM tag WHERE name LIKE ?;";
		q = "%" + q + "%";
	}
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, 0);
	QByteArray query_bytes = query.toUtf8();
	sqlite3_bind_text(stmt, 1, query_bytes.constData(), -1, SQLITE_STATIC);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		tags.append(fromStmt(stmt));
		//int64_t id = sqlite3_column_int64(stmt, 0);
		//if (m_instancesByID.contains(id))
		//	if (QSharedPointer<Tag> tag = m_instancesByID.value(id).toStrongRef())
		//	{
		//		tags.append(tag);
		//		continue;
		//	}
		//QSharedPointer<Tag> tag(new Tag(stmt));
		//tags.append(tag);
		//QWeakPointer<Tag> tag_weak = tag.toWeakRef();
		//m_instancesByID.insert(tag->id(), tag_weak);
		//m_instancesByName.insert(tag->name(), tag_weak);
	}
	sqlite3_finalize(stmt);
	return tags;
}

//int Tag::update(QSharedPointer<Tag>& tag, const QString& name, const QString& description, const QStringList& urls)
//{ }

//int Tag::update(const QList<QSharedPointer<Tag>>& tags, const QString& name, const QString& description, const QStringList& urls)
//{
//	QString urls_str;
//	if (urls.size() == 0)
//		urls_str = "";
//	else
//	{
//		QStringList urls_cpy;
//		for (QString url : urls)
//			urls_cpy.append(QUrl(url, QUrl::TolerantMode).toString());
//		urls_str = urls_cpy.join(';');
//	}
//	
//	int rc = 0;
//	db_begin();
//	sqlite3_stmt* stmt;
//	std::string sql;
//	for (QSharedPointer<Tag> tag : tags)
//	{
//		sql = "UPDATE tag SET name = ?, description = ?, urls = ? WHERE id = ?;";
//		sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
//		QByteArray name_ba = name.toUtf8();
//		QByteArray description_ba = description.toUtf8();
//		QByteArray urls_ba = urls_str.toUtf8();
//		sqlite3_bind_text(stmt, 1, name_ba.constData(), -1, SQLITE_STATIC);
//		sqlite3_bind_text(stmt, 2, description_ba.constData(), -1, SQLITE_STATIC);
//		sqlite3_bind_text(stmt, 3, urls_ba.constData(), -1, SQLITE_STATIC);
//		sqlite3_bind_int64(stmt, 4, tag->m_id);
//		int rc = sqlite3_step(stmt);
//		sqlite3_finalize(stmt);
//		if (rc != SQLITE_DONE)
//		{
//			db_rollback();
//			qCritical() << "Tag update failed:" << sqlite3_errstr(rc);
//			return rc;
//		}
//	}
//	db_commit();
//	for (QSharedPointer<Tag> tag : tags)
//		tag->fetch();
//	return rc;
//}

//void Tag::update(QString& name, QString& description, QStringList& urls)
//{
//	update(QList{ QSharedPointer<Tag>(this) }, QString & name, QString & description, QStringList & urls);
//}

//void Tag::cleanup()
//{
//    for (QWeakPointer<Tag> tag_ptr : m_instancesByID.values())
//    {
//        if (QSharedPointer<Tag> tag = tag_ptr.lock() && tag.)
//        {
//
//            delete tag.get();
//        }
//        m_instancesByID.remove(tag.get()->id());
//        m_instancesByName.remove(tag.get()->name());
//    }
//}

int Tag::remove(QList<QSharedPointer<Tag>> tags)
{
	if (!db->isOpen())
		return SQLITE_CANTOPEN;
	int rc = 0;
	db->begin();
	sqlite3_stmt* stmt;
	std::string sql = "DELETE FROM tag WHERE id = ?;";
	for (QSharedPointer<Tag> tag : tags)
	{
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, tag->m_id);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
		{
			db->rollback();
			qCritical() << "Failed to delete tag" << tag->m_name << ":" << sqlite3_errstr(rc);
			return rc;
		}
	}
	db->commit();
	return rc;
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

//std::ostream& operator<<(std::ostream& os, const Tag& tag)
//{
//    QByteArray ba = tag.m_name.toUtf8();
//    os << ba.constData();
//    return os;
//}

//bool operator==(const Tag& l, const Tag& r)
//{
//	return l.id() == r.id();
//}

QMap<int64_t, QWeakPointer<Tag>> Tag::m_instances;
