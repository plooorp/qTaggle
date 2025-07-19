#pragma once

#include <QWidget>
#include "app/tag.h"

namespace Ui
{
	class TagProperties;
}

class TagProperties : public QWidget
{
	Q_OBJECT

public:
	explicit TagProperties(const QSharedPointer<Tag>& tag, QWidget* parent = nullptr);
	explicit TagProperties(QWidget* parent = nullptr);
	~TagProperties();
	QSharedPointer<Tag> tag() const;
	void setTag(const QSharedPointer<Tag>& tag);
	void clear();

private:
	Ui::TagProperties* m_ui;
	QSharedPointer<Tag> m_tag;
	void populate();
	void depopulate();
};
