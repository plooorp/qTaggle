#include "paginator.h"
#include "ui_paginator.h"

#include "app/globals.h"

Paginator::Paginator(QWidget* parent)
	: Paginator(0, 0, 9, parent)
{}

Paginator::Paginator(int currentPage, int minPage, int maxPage, QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::Paginator)
	, m_page(currentPage)
	, m_minPage(minPage)
	, m_maxPage(maxPage)
{
	Q_ASSERT(m_minPage <= m_page);
	Q_ASSERT(m_page <= m_maxPage);
	m_ui->setupUi(this);
	m_ui->spinBox->setMinimum(minPage + 1);
	m_ui->spinBox->setMaximum(maxPage + 1);

	m_ui->first->setIcon(QIcon(":/icons/page-first.svg"));
	m_ui->prev->setIcon(QIcon(":/icons/page-previous.svg"));
	m_ui->next->setIcon(QIcon(":/icons/page-next.svg"));
	m_ui->last->setIcon(QIcon(":/icons/page-last.svg"));

	connect(m_ui->first, &QToolButton::clicked, this, [this]() -> void { if (setPage(m_minPage)) emit pageChangedByUser(m_page); });
	connect(m_ui->prev, &QToolButton::clicked, this, [this]() -> void { if (setPage(m_page - 1)) emit pageChangedByUser(m_page); });
	connect(m_ui->next, &QToolButton::clicked, this, [this]() -> void { if (setPage(m_page + 1)) emit pageChangedByUser(m_page); });
	connect(m_ui->last, &QToolButton::clicked, this, [this]() -> void { if (setPage(m_maxPage)) emit pageChangedByUser(m_page); });
	connect(m_ui->spinBox, &QSpinBox::editingFinished, this, [this]() -> void { if (setPage(m_ui->spinBox->value() - 1)) emit pageChangedByUser(m_page); });
	
	updateLabel();
	updateButtons();
}

Paginator::~Paginator()
{
	delete m_ui;
}

int Paginator::page() const
{
	return m_page;
}

bool Paginator::setPage(int page)
{
	if (page == m_page)
		return false;

	if (page < m_minPage)
		m_page = m_minPage;
	else if (page > m_maxPage)
		m_page = m_maxPage;
	else
		m_page = page;
	
	updateLabel();
	updateButtons();
	emit pageChanged(m_page);
	return true;
}

int Paginator::minPage() const
{
	return m_minPage;
}

void Paginator::setMinPage(int minPage)
{
	Q_ASSERT(minPage <= m_maxPage);
	if (m_page < minPage)
		setPage(minPage);
	m_minPage = minPage;
	m_ui->spinBox->setMinimum(m_minPage + 1);
	updateLabel();
	updateButtons();
}

int Paginator::maxPage() const
{
	return m_maxPage;
}

void Paginator::setMaxPage(int maxPage)
{
	Q_ASSERT(m_minPage <= maxPage);
	if (m_page > maxPage)
		setPage(maxPage);
	m_maxPage = maxPage;
	m_ui->spinBox->setMaximum(m_maxPage + 1);
	updateLabel();
	updateButtons();
}

inline void Paginator::updateLabel()
{
	m_ui->label->setText(QLocale().toString(m_page + 1) + u"/"_s + QLocale().toString(m_maxPage + 1));
}

inline void Paginator::updateButtons()
{
	if (m_minPage == m_maxPage)
	{
		m_ui->first->setEnabled(false);
		m_ui->prev->setEnabled(false);
		m_ui->next->setEnabled(false);
		m_ui->last->setEnabled(false);
	}
	else if (m_page == m_minPage)
	{
		m_ui->first->setEnabled(false);
		m_ui->prev->setEnabled(false);
		m_ui->next->setEnabled(true);
		m_ui->last->setEnabled(true);
	}
	else if (m_page == m_maxPage)
	{
		m_ui->first->setEnabled(true);
		m_ui->prev->setEnabled(true);
		m_ui->next->setEnabled(false);
		m_ui->last->setEnabled(false);
	}
	else
	{
		m_ui->first->setEnabled(true);
		m_ui->prev->setEnabled(true);
		m_ui->next->setEnabled(true);
		m_ui->last->setEnabled(true);
	}
}

int Paginator::itemsPerPage() const
{
	return m_itemsPerPage;
}

void Paginator::setItemsPerPage(int items)
{
	if (items < 1)
		return;
	m_itemsPerPage = items;
	updateLabel();
}
