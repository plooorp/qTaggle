#pragma once

#include <QDialog>
#include <QList>
#include <QSharedPointer>
#include "app/tag.h"

namespace Ui
{
	class EditTagDialog;
}

class EditTagDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditTagDialog(const Tag& tag, QWidget* parent = nullptr);
	virtual ~EditTagDialog() override;

private slots:
	void accept() override;

private:
	Ui::EditTagDialog* m_ui;
	Tag m_tag;
};