#include "tagproperties.h"
#include "ui_tagproperties.h"

#include <QLocale>

TagProperties::TagProperties(const Tag& tag, QWidget* parent)
	: TagProperties(parent)
{
	setTag(tag);
}

TagProperties::TagProperties(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::TagProperties)
{
	m_ui->setupUi(this);
}

TagProperties::~TagProperties()
{
	delete m_ui;
}

void TagProperties::populate()
{
	m_ui->name->setText(m_tag.name());
	QString urls = "<html>";
	for (const QString& url : m_tag.urls())
		urls += u"<div>&bull; <a href=\"%1\">%1</a></div>"_s.arg(url);
	urls += "</html>";
	QCursor cursor = m_ui->urls->cursor();
	m_ui->urls->setText(urls);
	m_ui->description->setText(m_tag.description());
	m_ui->degree->setText(QLocale().toString(m_tag.degree()));
	m_ui->id->setText(QString::number(m_tag.id()));
	m_ui->modified->setText(QLocale().toString(m_tag.modified(), QLocale::ShortFormat));
	m_ui->modified->setToolTip(QLocale().toString(m_tag.modified(), QLocale::LongFormat));
	m_ui->created->setText(QLocale().toString(m_tag.created(), QLocale::ShortFormat));
	m_ui->created->setToolTip(QLocale().toString(m_tag.created(), QLocale::LongFormat));
}

void TagProperties::depopulate()
{
	m_ui->name->clear();
	m_ui->urls->clear();
	m_ui->description->clear();
	m_ui->degree->clear();
	m_ui->id->clear();
	m_ui->modified->clear();
	m_ui->created->clear();
}

Tag TagProperties::tag() const
{
	return m_tag;
}

void TagProperties::setTag(const Tag& tag)
{
	m_tag = tag;
	if (m_tag.exists())
		populate();
	else
		depopulate();
}

void TagProperties::clear()
{
	setTag(Tag());
}
