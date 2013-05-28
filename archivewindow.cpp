#include "archivewindow.h"
#include <QAction>
#include <QBoxLayout>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileSystemModel>
#include <QListWidget>
#include <QPushButton>
#include <QToolBar>
#include <QUrl>

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QDialog(parent)
{
    setMinimumSize(QSize(360, 360));

    QBoxLayout* layoutMain = new QVBoxLayout;

    QToolBar* barArchive = new QToolBar(tr("archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    barArchive->addAction(QIcon(":buttons/gallery"), tr("Gallery"), this, SLOT(onShowGalleryClick()));
    barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));
    layoutMain->addWidget(barArchive);

    barPath = new QToolBar(tr("path"));
    layoutMain->addWidget(barPath);

    listFiles = new QListWidget;
    listFiles->setFocusPolicy(Qt::NoFocus);
    connect(listFiles, SIGNAL(currentTextChanged(QString)), this, SLOT(listItemSelected(QString)));
    connect(listFiles, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(listItemDblClicked(QModelIndex)));
    layoutMain->addWidget(listFiles);
    QPushButton* btnClose = new QPushButton(tr("Close"));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));
    layoutMain->addWidget(btnClose, 1, Qt::AlignRight);
    setLayout(layoutMain);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void ArchiveWindow::setRoot(const QString& path)
{
    root.setPath(path);
}

void ArchiveWindow::setPath(const QString& path)
{
    curr.setPath(path);
    updatePathBar();
}

void ArchiveWindow::updatePathBar()
{
    QAction* prev = nullptr;
    QDir dir = curr;
    QActionGroup* group = new QActionGroup(barPath);
    connect(group, SIGNAL(triggered(QAction*)), this, SLOT(selectPath(QAction*)));
    barPath->clear();
    do
    {
        QAction* action = new QAction(dir.dirName(), barPath);
        action->setData(dir.absolutePath());
        action->setCheckable(true);
        action->setChecked(dir == curr);
        action->setActionGroup(group);
        barPath->insertAction(prev, action);
        prev = action;
    } while (dir != root && dir.cdUp());

    updateList();
}

void ArchiveWindow::updateList()
{
    qDebug() << curr.absolutePath();
    listFiles->clear();
    QFlags<QDir::Filter> filter = QDir::NoDot | QDir::AllEntries;
    if (curr == root)
    {
        filter |= QDir::NoDotDot;
    }
    listFiles->addItems(curr.entryList(filter));
}

void ArchiveWindow::selectPath(QAction* action)
{
    setPath(action->data().toString());
}

void ArchiveWindow::listItemSelected(QString item)
{
    if (item.isEmpty())
    {
        return;
    }

    QFileInfo fi(curr.absoluteFilePath(item));
    if (fi.isDir())
    {
        setPath(fi.absoluteFilePath());
    }
}

void ArchiveWindow::onShowGalleryClick()
{
    onShowFolderClick();
}

void ArchiveWindow::onShowFolderClick()
{
    QString url = listFiles->currentItem()? curr.absoluteFilePath(listFiles->currentItem()->text()): curr.absolutePath();
    QDesktopServices::openUrl(QUrl(url));
}

void ArchiveWindow::listItemDblClicked(QModelIndex)
{
    onShowFolderClick();
}
