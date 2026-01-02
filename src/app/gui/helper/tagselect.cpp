#include "tagselect.h"
#include "ui_tagselect.h"

#include <QCompleter>
#include <QStringListModel>
#include <QMessageBox>
#include <QMenu>
#include <QClipboard>
#include <QInputDialog>
#include <QSettings>

#include "app/database.h"

TagSelect::TagSelect(QWidget *parent)
	: TagSelect(QList<Tag>(), parent)
{}

TagSelect::TagSelect(const QList<Tag>& tags, QWidget* parent)
	: m_ui(new Ui::TagSelect)
	, m_model(new TagTableModel(this))
{
	m_ui->setupUi(this);

	connect(m_ui->buttonAdd, &QPushButton::clicked, this, &TagSelect::add);
	connect(m_ui->actionRemove, &QAction::triggered, this, &TagSelect::remove);
	connect(m_ui->lineEdit, &QLineEdit::returnPressed, this, &TagSelect::add);
	//connect(m_ui->lineEdit, &QLineEdit::returnPressed, this, [this]() -> void
	//	{
	//		if (!m_ui->lineEdit->completer()->popup()->isVisible())
	//			TagSelect::add();
	//	});
	//connect(m_ui->lineEdit->completer(), QOverload<const QString& >::of(&QCompleter::activated), this, &TagSelect::add);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TagSelect::remove);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &TagSelect::showContextMenu);

	connect(m_ui->buttonImport, &QPushButton::clicked, this, &TagSelect::importTags);
	connect(m_ui->buttonExport, &QPushButton::clicked, this, &TagSelect::exportTags);

	//TagCompleterModel* suggestions = new TagCompleterModel();
	//QCompleter* completer = new QCompleter(suggestions, this);
	//completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	//m_ui->lineEdit->setCompleter(completer);
	//connect(m_ui->lineEdit, &QLineEdit::textEdited, this
	//	, [this, suggestions]() -> void { suggestions->updateSuggestions(m_ui->lineEdit->text().split(' ').last()); });

	//TagSuggestions* suggestions = new TagSuggestions();
	//QCompleter* completer = new QCompleter(suggestions, this);
	//completer->setCaseSensitivity(Qt::CaseInsensitive);
	//completer->setCompletionMode(QCompleter::PopupCompletion);
	//completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
	//QStringList names;
	//sqlite3_stmt* stmt;
	//sqlite3_prepare_v2(db->con(), "SELECT name FROM tag ORDER BY name ASC;", -1, &stmt, nullptr);
	//while (sqlite3_step(stmt) == SQLITE_ROW)
	//	names.append(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0)));
	//sqlite3_finalize(stmt);
	//QStringListModel* completerModel = new QStringListModel(names, completer);
	//completer->setModel(completerModel);
	//m_ui->lineEdit->setCompleter(completer);

	if (!tags.isEmpty())
		setTags(tags);
	m_ui->treeView->setModel(m_model);

	readSettings();
}

TagSelect::~TagSelect()
{
	writeSettings();
	delete m_ui;
}

void TagSelect::add()
{
	const QString query = m_ui->lineEdit->text()
		.trimmed()
		.toLower();
	const Tag tag = Tag::fromName(query);
	// use this instead of tag.exists because Tag::fromName already performs a
	// query that verifies the tag does exist
	if (tag.id() < 0)
	{
		QMessageBox::warning(this, qApp->applicationName(), tr("Unknown tag: %1").arg(query));
		return;
	}
	if (m_model->contains(tag))
	{
		QMessageBox::warning(this, qApp->applicationName(), tr("%1 is already selected").arg(query));
		return;
	}
	m_model->addTag(tag);
	m_ui->lineEdit->clear();
}

void TagSelect::remove()
{
	QModelIndexList selected = m_ui->treeView->selectionModel()->selectedRows();
	std::sort(selected.begin(), selected.end()
		, [](const QModelIndex& a, const QModelIndex& b) -> bool
		{
			return a.row() > b.row();
		});
	for (const QModelIndex& index : selected)
		m_model->removeTag(index.row());
}

QList<Tag> TagSelect::tags()
{
	return m_model->tags();
}

void TagSelect::setTags(const QList<Tag>& tags)
{
	m_model->clear();
	for (const Tag& tag : tags)
		m_model->addTag(tag);
}

void TagSelect::showContextMenu(const QPoint& pos)
{
	if (!m_ui->treeView->indexAt(pos).isValid())
		m_ui->treeView->clearSelection();
	if (m_ui->treeView->selectionModel()->selectedRows().isEmpty())
		return;
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	menu->addAction(m_ui->actionRemove);
	menu->popup(QCursor::pos());
}

void TagSelect::importTags()
{
	QInputDialog dialog(this);
	dialog.setWindowTitle(tr("Import tags"));
	dialog.setLabelText(tr("Paste tags below, space delimited."));
	dialog.setOptions(QInputDialog::UsePlainTextEditForTextInput);
	if (dialog.exec())
	{
		QStringList invalidTags;
		for (const QString& name : dialog.textValue().trimmed().split(' '))
		{
			Tag tag = Tag::fromName(name);
			if (tag.exists())
			{
				if (!m_model->contains(tag))
					m_model->addTag(tag);
			}
			else
				invalidTags.append(name);
		}
		if (!invalidTags.isEmpty())
			QMessageBox::warning(this, tr("Some or all tags failed to import")
				, tr("The following tags do not exist: ") + invalidTags.join(", "));
	}
}

void TagSelect::exportTags()
{
	QClipboard* clipboard = qApp->clipboard();
	QStringList tags;
	for (const Tag& tag : m_model->tags())
		tags.append(tag.name());
	clipboard->setText(tags.join(' '));
}

void TagSelect::readSettings()
{
	QSettings settings;
	const QByteArray headerState = settings.value("GUI/TagSelect/headerState", QByteArray()).toByteArray();
	if (!headerState.isEmpty())
		m_ui->treeView->header()->restoreState(headerState);
}

void TagSelect::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/TagSelect/headerState", m_ui->treeView->header()->saveState());
}
