#pragma once

#include <QWidget>

namespace Ui
{
	class Paginator;
}

class Paginator : public QWidget
{
	Q_OBJECT

public:
	explicit Paginator(QWidget* parent);
	explicit Paginator(int currentPage, int minPage, int maxPage, QWidget* parent = nullptr);
	~Paginator();
	int page() const;
	bool setPage(int);
	int minPage() const;
	void setMinPage(int);
	int maxPage() const;
	void setMaxPage(int);

signals:
	void customPageSubmitted(int page);
	void pageChanged(int page);
	void pageChangedByUser(int page);

private:
	Ui::Paginator* m_ui;
	int m_page;
	int m_maxPage;
	int m_minPage;
	inline void updateLabel();
	inline void updateButtons();
};
