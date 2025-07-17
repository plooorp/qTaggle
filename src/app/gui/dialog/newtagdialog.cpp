#include "newtagdialog.h"
#include "ui_newtagdialog.h"

#include "app/tag.h"

NewTagDialog::NewTagDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , m_ui(new Ui::NewTagDialog)
{
    m_ui->setupUi(this);
}

NewTagDialog::~NewTagDialog()
{
    delete m_ui;
}

void NewTagDialog::accept()
{
    QSharedPointer<Tag> tag = Tag::create(m_ui->nameLineEdit->text()
        , m_ui->descriptionPlainTextEdit->toPlainText(), m_ui->urlsPlainTextListEdit->values());
    QDialog::accept();
}

void NewTagDialog::reject()
{
    QDialog::reject();
}
