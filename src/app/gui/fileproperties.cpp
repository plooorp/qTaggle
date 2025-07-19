#include "fileproperties.h"
#include "ui_fileproperties.h"

#include <QLocale>

FileProperties::FileProperties(const QSharedPointer<File>& file, QWidget* parent)
	: FileProperties(parent)
{
	setFile(file);
}

FileProperties::FileProperties(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::FileProperties)
{
	m_ui->setupUi(this);
}

FileProperties::~FileProperties()
{
	delete m_ui;
}

void FileProperties::populate()
{
	m_ui->name->setText(m_file->name());
	m_ui->state->setText(File::stateString(m_file->state()));
	m_ui->comment->setText(m_file->comment());
	QStringList tags;
	for (const FileTag& ft : m_file->tags())
		tags.append(ft.tag()->name());
	m_ui->tags->setText(tags.join(", "));
	m_ui->source->setText("<html><a href=\"" + m_file->source() + "\">" + m_file->source() + "</a></html>");
	m_ui->id->setText(QString::number(m_file->id()));
	m_ui->path->setText(m_file->path());
	m_ui->sha1->setText(QString(m_file->sha1().toHex()));
	m_ui->modified->setText(QLocale().toString(m_file->modified(), QLocale::ShortFormat));
	m_ui->modified->setToolTip(QLocale().toString(m_file->modified(), QLocale::LongFormat));
	m_ui->created->setText(QLocale().toString(m_file->created(), QLocale::ShortFormat));
	m_ui->created->setToolTip(QLocale().toString(m_file->created(), QLocale::LongFormat));
}

void FileProperties::depopulate()
{
	m_ui->name->clear();
	m_ui->state->clear();
	m_ui->comment->clear();
	m_ui->tags->clear();
	m_ui->source->clear();
	m_ui->id->clear();
	m_ui->path->clear();
	m_ui->sha1->clear();
	m_ui->modified->clear();
	m_ui->created->clear();
}

QSharedPointer<File> FileProperties::file() const
{
	return m_file;
}

void FileProperties::setFile(const QSharedPointer<File>& file)
{
	m_file = file;
	if (m_file)
		populate();
	else
		depopulate();
}

void FileProperties::clear()
{
	setFile(QSharedPointer<File>());
}
