#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QSharedPointer>
#include <QWeakPointer>

#include "database.h"

class Tag final : public QObject, public Record
{
	Q_OBJECT

public:
	~Tag();
	static DBResult create(const QString& name, const QString& description = QString()
		, const QStringList& urls = QStringList(), QSharedPointer<Tag>* out = nullptr);
	static QSharedPointer<Tag> fromStmt(sqlite3_stmt* stmt);
	static QSharedPointer<Tag> fromID(const int64_t id);
	static QSharedPointer<Tag> fromName(const QString& name);
	void fetch();
	int64_t id() const;
	QString name() const;
	DBResult setName(const QString& name);
	QString description() const;
	DBResult setDescription(const QString& description);
	QStringList urls() const;
	DBResult setURLs(const QStringList& urls);
	int64_t degree() const;
	QDateTime created() const;
	QDateTime modified() const;
	DBResult remove();

signals:
	void updated();
	void deleted();

private:
	Tag(sqlite3_stmt* stmt);
	static QMap<int64_t, QWeakPointer<Tag>> m_instances;
	static inline QString normalizeName(const QString& name);
	int64_t m_id;
	QString m_name;
	QString m_description;
	QList<QString> m_urls;
	int64_t m_degree;
	QDateTime m_created;
	QDateTime m_modified;
};
