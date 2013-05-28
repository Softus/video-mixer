#include "archivewindow.h"
#include <QAction>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QDir>
#include <QFileSystemModel>
#include <QListWidget>
#include <QToolBar>
#include <QUrl>

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QDialog(parent)
{
    setMinimumSize(QSize(200, 200));

    QBoxLayout* layoutMain = new QVBoxLayout;

    QToolBar* barArchive = new QToolBar(tr("archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    barArchive->addAction(QIcon(":buttons/gallery"), tr("Gallery"), this, SLOT(onShowGalleryClick()));
    barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));
    layoutMain->addWidget(barArchive);

    barPath = new QToolBar(tr("path"));
    layoutMain->addWidget(barPath);

    listFiles = new QListWidget;
    listFiles->setCurrentRow(-1);
    connect(listFiles, SIGNAL(currentTextChanged(QString)), this, SLOT(listItemSelected(QString)));
    layoutMain->addWidget(listFiles);
    setLayout(layoutMain);
}

void ArchiveWindow::setRoot(const QDir& path)
{
    root = path;
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

    updateList(curr);
}

void ArchiveWindow::updateList(QDir dir)
{
    listFiles->clear();
    QFlags<QDir::Filter> filter = QDir::NoDot | QDir::AllEntries;
    if (dir == root)
    {
        filter |= QDir::NoDotDot;
    }
    listFiles->setFocusPolicy(Qt::NoFocus);
    listFiles->addItems(dir.entryList(filter));
}

void ArchiveWindow::selectPath(QAction* action)
{
    curr.setPath(action->data().toString());
    updateList(curr);
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
