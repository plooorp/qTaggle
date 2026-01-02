#include "fileproperties.h"
#include "ui_fileproperties.h"

#include <QLocale>

FileProperties::FileProperties(const File& file, QWidget* parent)
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
	QString alias = m_file.alias();
	if (alias.isEmpty())
		m_ui->name->setText(m_file.name());
	else
		m_ui->name->setText(alias + u" ("_s +  m_file.name() + u")"_s);
	m_ui->state->setText(File::stateString[m_file.state()]);
	m_ui->comment->setText(m_file.comment());
	QStringList tags;
	for (const FileTag& ft : m_file.tags())
		tags.append(Tag(ft.tagId()).name());
	m_ui->tags->setText(tags.join(", "));
	m_ui->source->setText(u"<a href=\"%1\">%1</a>"_s.arg(m_file.source()));
	m_ui->id->setText(QString::number(m_file.id()));
	m_ui->path->setText(m_file.path());
	m_ui->sha1->setText(QString(m_file.sha1().toHex()));
	m_ui->modified->setText(QLocale().toString(m_file.modified(), QLocale::ShortFormat));
	m_ui->modified->setToolTip(QLocale().toString(m_file.modified(), QLocale::LongFormat));
	m_ui->created->setText(QLocale().toString(m_file.created(), QLocale::ShortFormat));
	m_ui->created->setToolTip(QLocale().toString(m_file.created(), QLocale::LongFormat));
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

File FileProperties::file() const
{
	return m_file;
}

void FileProperties::setFile(const File& file)
{
	m_file = file;
	if (m_file.exists())
		populate();
	else
		depopulate();
}

void FileProperties::clear()
{
	setFile(File());
}
