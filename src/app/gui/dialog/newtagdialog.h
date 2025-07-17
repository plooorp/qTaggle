#pragma once

#include <QtWidgets/QDialog>

namespace Ui
{
    class NewTagDialog;
}

class NewTagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewTagDialog(QWidget *parent = nullptr, Qt::WindowFlags f = { 0 });
    ~NewTagDialog();

private slots:
    void accept() override;
    void reject() override;

private:
    Ui::NewTagDialog* m_ui;
};

