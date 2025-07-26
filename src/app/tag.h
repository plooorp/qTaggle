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
	static QSharedPointer<Tag> create(const QString& name, const QString& description, const QStringList& urls);
	static QSharedPointer<Tag> create(const QString& name, const QString& description);
	static QSharedPointer<Tag> create(const QString& name);
	static QSharedPointer<Tag> fromStmt(sqlite3_stmt* stmt);
	static QSharedPointer<Tag> fromID(const int64_t id);
	static QList<QSharedPointer<Tag>> fromQuery(const QString& query);
	void fetch();
	static int remove(QList<QSharedPointer<Tag>> tags);
	//int remove();
	//static void cleanup();
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
	//friend std::ostream& operator<<(std::ostream& os, const Tag& tag);
	//inline bool operator==(const Tag& l, const Tag& r);

signals:
	void updated();

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
