#pragma once

#include <QDialog>
#include "app/tag.h"

namespace Ui
{
	class EditTagDialogMulti;
}

class EditTagDialogMulti : public QDialog
{
	Q_OBJECT

public:
	explicit EditTagDialogMulti(const QList<Tag>& tags, QWidget* parent = nullptr);
	virtual ~EditTagDialogMulti() override;

private slots:
	void accept() override;

private:
	Ui::EditTagDialogMulti* m_ui;
	QList<Tag> m_tags;
};
