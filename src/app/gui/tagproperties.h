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
	explicit TagProperties(const Tag& tag, QWidget* parent = nullptr);
	explicit TagProperties(QWidget* parent = nullptr);
	virtual ~TagProperties() override;
	Tag tag() const;
	void setTag(const Tag& tag);
	void clear();

private:
	Ui::TagProperties* m_ui;
	Tag m_tag;
	void populate();
	void depopulate();
};
