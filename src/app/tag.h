#pragma once

#include <QDateTime>

#include "database.h"

struct Tag
{
public:
	Tag();
	Tag(int64_t id);
	static DBError create(const QString& name, const QString& description = QString(), const QList<QString>& urls = QList<QString>(), Tag* out = nullptr);
	static Tag fromName(const QString& name);
	bool exists() const;
	int64_t id() const;
	QString name() const;
	QString description() const;
	QStringList urls() const;
	int64_t degree() const;
	QDateTime created() const;
	QDateTime modified() const;
	DBError setName(const QString& name) const;
	DBError setDescription(const QString& description) const;
	DBError addURL(const QString& url) const;
	DBError removeURL(const QString& url) const;
	DBError setURLs(const QStringList& urls) const;
	DBError remove() const;
	bool operator==(const Tag& other) const
	{
		return this->id() == other.id();
	}
	bool operator!=(const Tag& other) const
	{
		return this->id() == other.id();
	}

private:
	static inline QString normalizeName(const QString& name);
	int64_t m_id;
	DBError updateModified() const;
};

namespace std
{
	template <>
	struct hash<Tag>
	{
		std::size_t operator()(const Tag& tag) const noexcept
		{
			return tag.id();
		}
	};
}
