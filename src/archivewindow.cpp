/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "archivewindow.h"
#include "patientdatadialog.h"
#include "defaults.h"
#include "smartshortcut.h"
#include "qwaitcursor.h"
#include "thumbnaillist.h"
#include "typedetect.h"

#ifdef WITH_DICOM
#include "dicom/dcmclient.h"
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#endif

#include "touch/slidingstackedwidget.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileIconProvider>
#include <QFileSystemWatcher>
#include <QFocusEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPainter>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStringList>
#include <QSlider>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QxtConfirmationMessage>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/Caps>
#include <QGst/Event>
#include <QGst/Pad>
#include <QGst/Parse>
#include <QGst/Query>
#include <QGst/Ui/VideoWidget>

#include <gst/gstdebugutils.h>

#ifdef Q_OS_WIN
  #include <qt_windows.h>
#endif

#define GALLERY_MODE 2

static QSize videoSize(352, 258);

static QStringList collectRemovableDrives()
{
    QStringList ret;
    QList<QDBusObjectPath> removableDrives;
    QDBusReply<QList<QDBusObjectPath> > reply = QDBusConnection::systemBus().call(
        QDBusMessage::createMethodCall("org.freedesktop.UDisks", "/org/freedesktop/UDisks",
                                       "org.freedesktop.UDisks", "EnumerateDevices"));
    if (reply.isValid())
    {
        auto paths = reply.value();

        // Collect all removable drives
        //
        foreach (auto path, paths)
        {
            QDBusInterface iface("org.freedesktop.UDisks", path.path(),
                                     "org.freedesktop.DBus.Properties", QDBusConnection::systemBus());
            if (!iface.isValid())
                continue;

            QDBusReply<QVariant> reply = iface.call("Get", "org.freedesktop.UDisks.Device", "DeviceIsRemovable");
            if (reply.isValid() && reply.value().toBool())
            {
                removableDrives << path;
            }
        }

        // Now find all partitions belongs to removable drives collected before.
        //
        foreach (auto path, paths)
        {
            QDBusInterface iface("org.freedesktop.UDisks", path.path(),
                                     "org.freedesktop.DBus.Properties", QDBusConnection::systemBus());

            if (!iface.isValid())
                continue;

            QDBusReply<QVariant> reply = iface.call("Get", "org.freedesktop.UDisks.Device", "PartitionSlave");
            if (!reply.isValid() || !removableDrives.contains(reply.value().value<QDBusObjectPath>()))
            {
                // Not a partition of a removable drive
                //
                continue;
            }

            reply = iface.call("Get", "org.freedesktop.UDisks.Device", "DeviceIsReadOnly");
            if (!reply.isValid() || reply.value().toBool())
            {
                // ReadOnly or not ready (password protected, etc)
                //
                continue;
            }

            reply = iface.call("Get", "org.freedesktop.UDisks.Device", "DeviceMountPaths");
            if (reply.isValid() && !reply.value().toString().isEmpty())
            {
                // Save mount path
                //
                ret.append(reply.value().toString());
            }
        }
    }
#ifdef Q_OS_WIN
    else
    {
        Uint32 dummy = 0, drives = GetLogicalDrives();
        wchar_t drvPath[] = {'0', ':', '\\', '\x0'};

        for (int i = 0; i < 26; ++i)
        {
            if (!(drives & (1 << i)))
                continue;

            drvPath[0] = 'A' + i;
            if (GetDriveTypeW(drvPath) == 2 && GetDiskFreeSpaceW(drvPath, &dummy, &dummy, &dummy, &dummy))
            {
                ret.append(QString::fromWCharArray(drvPath));
            }
        }
    }
#endif

    return ret;
}

ArchiveWindow::ArchiveWindow(QWidget *parent)
    : QWidget(parent)
    , updateTimerId(0)
    , updateUsbTimerId(0)
{
    QSettings settings;
    root = QDir::root();

    dirWatcher = new QFileSystemWatcher(this);
    connect(dirWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(onDirectoryChanged(const QString &)));

    auto layoutMain = new QVBoxLayout;

    auto barArchive = new QToolBar(tr("Archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    actionBack = barArchive->addAction(QIcon(":buttons/back"), tr("Back"), this, SLOT(onBackToMainWindowClick()));

    actionDelete = barArchive->addAction(QIcon(":buttons/delete"), tr("Delete"), this, SLOT(onDeleteClick()));
    actionDelete->setEnabled(false);
    actionRestore = barArchive->addAction(QIcon(":/buttons/undo"), tr("Restore"), this, SLOT(onRestoreClick()));
    actionRestore->setEnabled(false);

#ifdef WITH_DICOM
    actionStore = barArchive->addAction(QIcon(":buttons/dicom"), tr("Upload"), this, SLOT(onStoreClick()));
#endif
    btnUsbStore = new QToolButton;
    btnUsbStore->setIcon(QIcon(":buttons/usb"));
    btnUsbStore->setText(tr("to USB"));
    btnUsbStore->setToolButtonStyle(barArchive->toolButtonStyle());
    btnUsbStore->setPopupMode(QToolButton::InstantPopup);
    connect(btnUsbStore, SIGNAL(clicked()), this, SLOT(onUsbStoreClick()));
    connect(btnUsbStore, SIGNAL(triggered(QAction*)), this, SLOT(onUsbStoreMenuClick(QAction*)));
    barArchive->addWidget(btnUsbStore);

    actionEdit = barArchive->addAction(QIcon(":buttons/edit"), tr("Edit"), this, SLOT(onEditClick()));
    actionEdit->setEnabled(false);
    actionUp = barArchive->addAction(QIcon(":/buttons/up"), tr("Up"), this, SLOT(onUpFolderClick()));
    actionUp->setEnabled(false);

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barArchive->addWidget(spacer);

    actionMode = new QAction(barArchive);
    connect(actionMode, SIGNAL(triggered()), this, SLOT(onSwitchModeClick()));

    auto menuMode = new QMenu;
    connect(menuMode, SIGNAL(triggered(QAction*)), this, SLOT(onSwitchModeClick(QAction*)));
    actionListMode = menuMode->addAction(QIcon(":buttons/list"), tr("List"));
    actionListMode->setData(QListView::ListMode);
    actionIconMode = menuMode->addAction(QIcon(":buttons/icons"), tr("Icons"));
    actionIconMode->setData(QListView::IconMode);
    actionGalleryMode = menuMode->addAction(QIcon(":buttons/gallery"), tr("Gallery"));
    actionGalleryMode->setData(GALLERY_MODE);

    actionMode->setMenu(menuMode);
    barArchive->addAction(actionMode);

    actionBrowse = barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));


    barPath = new QToolBar(tr("Path"));
    layoutMain->addWidget(barPath);

    player = new QWidget;

    auto playerLayout = new QVBoxLayout();
    playerLayout->setContentsMargins(0,16,0,16);

    auto playerInnerLayout = new QHBoxLayout();
    playerInnerLayout->setContentsMargins(0,0,0,0);

    auto barPrev = new QToolBar(tr("Previous"));
    auto btnPrev = new QToolButton;
    btnPrev->setIcon(QIcon(":buttons/prev"));
    btnPrev->setText(tr("Previous"));
    btnPrev->setToolTip(btnPrev->text() + " (" + QKeySequence(Qt::Key_Left).toString() + ")");
    btnPrev->setMinimumHeight(200);
    btnPrev->setFocusPolicy(Qt::NoFocus);
    connect(btnPrev, SIGNAL(clicked()), this, SLOT(onPrevClick()));
    barPrev->addWidget(btnPrev);
    playerInnerLayout->addWidget(barPrev);

    // Add two display widgets: one is visible, one is loading.
    // Once loading is complete, we will switch them.
    //
    pagesWidget = new QStackedWidget;
    pagesWidget->setMinimumSize(videoSize);
    pagesWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pagesWidget->addWidget(new QGst::Ui::VideoWidget);
    pagesWidget->addWidget(new QGst::Ui::VideoWidget);

    playerInnerLayout->addWidget(pagesWidget);

    auto barNext = new QToolBar(tr("Next"));
    auto btnNext = new QToolButton;
    btnNext->setIcon(QIcon(":buttons/next"));
    btnNext->setText(tr("Next"));
    btnNext->setToolTip(btnNext->text() + " (" + QKeySequence(Qt::Key_Right).toString() + ")");
    btnNext->setMinimumHeight(200);
    btnNext->setFocusPolicy(Qt::NoFocus);
    connect(btnNext, SIGNAL(clicked()), this, SLOT(onNextClick()));
    barNext->addWidget(btnNext);
    playerInnerLayout->addWidget(barNext);

    playerLayout->addLayout(playerInnerLayout);

    barMediaControls = new QToolBar(tr("Media"));
    actionSeekBack = barMediaControls->addAction(QIcon(":buttons/rewind"), tr("Rewing"), this, SLOT(onSeekClick()));
    actionSeekBack->setVisible(false);
    actionSeekBack->setData(-40000000);
    actionPlay = barMediaControls->addAction(QIcon(":buttons/play"), tr("Play"), this, SLOT(onPlayPauseClick()));
    actionPlay->setVisible(false);
    actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  tr("Forward"), this, SLOT(onSeekClick()));
    actionSeekFwd->setVisible(false);
    actionSeekFwd->setData(+40000000);
    barMediaControls->setMinimumSize(48, 48);

    playerLayout->addWidget(barMediaControls, 0, Qt::AlignHCenter);

    player->setLayout(playerLayout);
    layoutMain->addWidget(player);

    listFiles = new ThumbnailList(true);
    listFiles->setSelectionMode(QListWidget::ExtendedSelection);
//    listFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
//    listFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listFiles->setMovement(QListView::Snap);
    connect(listFiles, SIGNAL(currentRowChanged(int)), this, SLOT(onListRowChanged(int)));
    connect(listFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onListItemDoubleClicked(QListWidgetItem*)));
    connect(listFiles, SIGNAL(itemDraggedOut(QListWidgetItem*)), this, SLOT(onListItemDraggedOut(QListWidgetItem*)));
    actionEnter = new QAction(listFiles);
    connect(actionEnter, SIGNAL(triggered()), this, SLOT(onListKey()));
    listFiles->addAction(actionEnter);
    layoutMain->addWidget(listFiles);

    layoutMain->addWidget(barArchive);
    setLayout(layoutMain);

    updateHotkeys(settings);

    QDBusConnection::systemBus().connect("org.freedesktop.UDisks", "/org/freedesktop/UDisks",
                                         "org.freedesktop.UDisks", "DeviceAdded",
                                         this, SLOT(onUsbDiskChanged()));
    QDBusConnection::systemBus().connect("org.freedesktop.UDisks", "/org/freedesktop/UDisks",
                                         "org.freedesktop.UDisks", "DeviceRemoved",
                                         this, SLOT(onUsbDiskChanged()));
    QDBusConnection::systemBus().connect("org.freedesktop.UDisks", "/org/freedesktop/UDisks",
                                         "org.freedesktop.UDisks", "DeviceChanged",
                                         this, SLOT(onUsbDiskChanged()));
}

void ArchiveWindow::updateHotkeys(QSettings& settings)
{
    settings.beginGroup("hotkeys");
    updateShortcut(actionBack,        settings.value("archive-back",          DEFAULT_HOTKEY_BACK).toInt());
    updateShortcut(actionDelete,      settings.value("archive-delete",        DEFAULT_HOTKEY_DELETE).toInt());
    updateShortcut(actionRestore,     settings.value("archive-restore",       DEFAULT_HOTKEY_RESTORE).toInt());
#ifdef WITH_DICOM
    updateShortcut(actionStore,       settings.value("archive-upload",        DEFAULT_HOTKEY_UPLOAD).toInt());
#endif
    updateShortcut(btnUsbStore,       settings.value("archive-usb",           DEFAULT_HOTKEY_USB).toInt());
    updateShortcut(actionEdit,        settings.value("archive-edit",          DEFAULT_HOTKEY_EDIT).toInt());
    updateShortcut(actionUp,          settings.value("archive-parent-folder", DEFAULT_HOTKEY_PARENT_FOLDER).toInt());

    updateShortcut(actionMode,        settings.value("archive-next-mode",     DEFAULT_HOTKEY_NEXT_MODE).toInt());
    updateShortcut(actionListMode,    settings.value("archive-list-mode",     DEFAULT_HOTKEY_LIST_MODE).toInt());
    updateShortcut(actionIconMode,    settings.value("archive-icon-mode",     DEFAULT_HOTKEY_ICON_MODE).toInt());
    updateShortcut(actionGalleryMode, settings.value("archive-gallery-mode",  DEFAULT_HOTKEY_GALLERY_MODE).toInt());

    updateShortcut(actionBrowse,      settings.value("archive-browse",        DEFAULT_HOTKEY_BROWSE).toInt());

    updateShortcut(actionSeekBack,    settings.value("archive-seek-back",     DEFAULT_HOTKEY_SEEK_BACK).toInt());
    updateShortcut(actionSeekFwd,     settings.value("archive-seek-fwd",      DEFAULT_HOTKEY_SEEK_FWD).toInt());
    updateShortcut(actionPlay,        settings.value("archive-play",          DEFAULT_HOTKEY_PLAY).toInt());
    updateShortcut(actionEnter,       settings.value("archive-select",        DEFAULT_HOTKEY_SELECT).toInt());

    // Not a real keys
    // updateShortcut(btnPrev,        settings.value("archive-prev",        DEFAULT_HOTKEY_PREV).toInt());
    // updateShortcut(btnNext,        settings.value("archive-next",        DEFAULT_HOTKEY_NEXT).toInt());
    settings.endGroup();
}

ArchiveWindow::~ArchiveWindow()
{
    stopMedia();
    reallyDeleteFiles();
}

void ArchiveWindow::showEvent(QShowEvent *evt)
{
    actionBack->setVisible(parent() != nullptr);
    onDirectoryChanged(QString());
    onUsbDiskChanged();
    QWidget::showEvent(evt);
}

void ArchiveWindow::timerEvent(QTimerEvent* evt)
{
    if (evt->timerId() == updateTimerId)
    {
        killTimer(updateTimerId);
        updateTimerId = 0;
        updatePath();
    }
    else if (evt->timerId() == updateUsbTimerId)
    {
        killTimer(updateUsbTimerId);
        updateUsbTimerId = 0;
        updateUsbStoreButton();
    }
}

void ArchiveWindow::updateRoot()
{
    QSettings settings;
    root.setPath(settings.value("storage/output-path", DEFAULT_OUTPUT_PATH).toString());
    if (!root.exists())
    {
        root.mkpath(".");
    }
    curr = root;
    switchViewMode(settings.value("ui/archive-mode").toInt());
}

void ArchiveWindow::setPath(const QString& path)
{
    if (curr.path() != path)
    {
        curr.setPath(path);
        actionUp->setEnabled(root != curr);

        dirWatcher->removePaths(dirWatcher->directories());
        dirWatcher->addPath(path);
        onDirectoryChanged(QString());
        reallyDeleteFiles();
    }
}

void ArchiveWindow::onRestoreClick()
{
    deleteLater.clear();
    actionRestore->setEnabled(false);
    updateList();
}

void ArchiveWindow::onUpFolderClick()
{
    auto pathActions = barPath->actions();
    auto size = pathActions.size();
    if (size > 1)
    {
        auto currFolderName = curr.dirName();
        setPath(pathActions.at(pathActions.size() - 2)->data().toString());
        selectFile(currFolderName);
    }
}

void ArchiveWindow::onDirectoryChanged(const QString&)
{
    killTimer(updateTimerId);
    updateTimerId = startTimer(200);
}

void ArchiveWindow::createSubDirMenu(QAction* parentAction)
{
    QDir dir(parentAction->data().toString());
    auto menu = new QMenu;

    foreach (auto subDir, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
    {
        auto action = menu->addAction(subDir.fileName(), this, SLOT(selectPath()));
        action->setData(subDir.absoluteFilePath());
        action->setCheckable(true);
    }

    if (!menu->isEmpty())
    {
        parentAction->setMenu(menu);
        connect(menu, SIGNAL(aboutToShow()), this, SLOT(preparePathPopupMenu()));
    }
}

void ArchiveWindow::updatePath()
{
    stopMedia();

    QAction* prev = nullptr;
    QDir dir = curr;

    auto group = new QActionGroup(barPath);
    connect(group, SIGNAL(triggered(QAction*)), this, SLOT(selectPath(QAction*)));
    barPath->clear();

    do
    {
        auto action = new QAction(dir.dirName(), barPath);
        action->setData(dir.absolutePath());
        action->setCheckable(true);
        action->setChecked(dir == curr);
        action->setActionGroup(group);
        createSubDirMenu(action);
        barPath->insertAction(prev, action);
        prev = action;
    } while (dir != root && dir.cdUp());

    if (isVisible())
    {
        updateList();
    }
}

void ArchiveWindow::preparePathPopupMenu()
{
    auto currPath = curr.absolutePath();
    auto menu = static_cast<QMenu*>(sender());
    foreach (auto action, menu->actions())
    {
        action->setChecked(currPath == action->data().toString());
        createSubDirMenu(action);
    }
}

static QString addStatusOverlay(const QString& filePath, QIcon& icon, const QString& attr, const QString& iconPath)
{
    //qDebug() << filePath;

    auto archiveStatus = GetFileExtAttribute(filePath, attr);
    if (!archiveStatus.isEmpty())
    {
        QPixmap pmOverlay;
        if (archiveStatus == "ok")
        {
            pmOverlay.load(iconPath);
        }
        else
        {
            pmOverlay = QMessageBox::standardIcon(QMessageBox::Critical);
        }

        auto sizes = icon.availableSizes();
        auto size = sizes.empty()? QSize(128,128): sizes.first();
        auto pm = icon.pixmap(size);
        QPainter painter(&pm);
        painter.setOpacity(0.75);
        auto rect = pm.rect();
        rect.setBottom(rect.top() + rect.height() / 4);
        rect.setRight(rect.left() + rect.width() / 4);
        painter.drawPixmap(rect, pmOverlay);
        icon.addPixmap(pm);
    }

    return archiveStatus;
}

void ArchiveWindow::updateList()
{
    QWaitCursor wait(this);
    QFileIconProvider fip;

    auto currText = listFiles->currentItem()? listFiles->currentItem()->text(): nullptr;

    listFiles->setUpdatesEnabled(false);
    listFiles->clear();
    auto filter = QDir::AllEntries | QDir::NoDotAndDotDot;

    foreach (QFileInfo fi, curr.entryInfoList(filter, QDir::Time | QDir::Reversed))
    {
        //qDebug() << fi.absoluteFilePath();
        QIcon icon;
        QPixmap pm;

        if (deleteLater.contains(fi.absoluteFilePath()))
        {
            // Got a file queued for deletion, skip it
            //
            continue;
        }
        else if (fi.isDir())
        {
            icon.addFile(":/buttons/folder");
        }
        else if (QFile::exists(curr.absoluteFilePath(fi.completeBaseName())))
        {
            // Got a thumbnail image, skip it
            //
            continue;
        }
        else
        {
            auto caps = TypeDetect(fi.absoluteFilePath());
            if (caps.startsWith("video/"))
            {
                auto thumbnailList = curr.entryInfoList(QStringList("." + fi.fileName() + ".*"), QDir::Hidden | QDir::Files);
                if (thumbnailList.isEmpty() || !pm.load(thumbnailList.first().absoluteFilePath()))
                {
                    icon.addFile(":/buttons/movie");
                }
                else
                {
                    // Got a snapshot for a clip file. Add a fency overlay to it
                    //
                    QPixmap pmOverlay(":/buttons/film");
                    QPainter painter(&pm);
                    painter.setOpacity(0.75);
                    painter.drawPixmap(pm.rect(), pmOverlay);
                    icon.addPixmap(pm);
                }
            }
            else if (pm.load(fi.absoluteFilePath()))
            {
                icon.addPixmap(pm);
            }
            else
            {
                icon.addPixmap(fip.icon(fi).pixmap(QSize(128,128))); // PDF and so on
            }
        }

        auto toolTip = fi.absoluteFilePath();

#ifdef WITH_DICOM
        auto dicomStatus = addStatusOverlay(fi.absoluteFilePath(),  icon, "dicom-status", ":/buttons/database");
        if (!dicomStatus.isEmpty())
        {
            if (dicomStatus == "ok")
            {
                dicomStatus = tr("The file was uploaded to a storage server");
            }

            toolTip += "\n" + dicomStatus;
        }
#endif

        auto usbStatus = addStatusOverlay(fi.absoluteFilePath(),  icon, "usb-status", ":/buttons/usb");
        if (!usbStatus.isEmpty())
        {
            if (usbStatus == "ok")
            {
                usbStatus = tr("The file was copied to an usb drive");
            }

            toolTip += "\n" + usbStatus;
        }

        auto item = new QListWidgetItem(icon, fi.fileName(), listFiles);
        item->setToolTip(toolTip);
        if (listFiles->viewMode() != QListView::ListMode)
        {
            item->setSizeHint(QSize(176, 144));
        }
        if (item->text() == currText)
        {
            listFiles->setCurrentItem(item);
        }
    }

    if (listFiles->currentRow() < 0)
    {
        listFiles->setCurrentRow(0);
    }
    listFiles->setUpdatesEnabled(true);
}

void ArchiveWindow::selectPath(QAction* action)
{
    setPath(action->data().toString());
}

void ArchiveWindow::selectPath()
{
    selectPath(static_cast<QAction*>(sender()));
}

void ArchiveWindow::selectFile(const QString& fileName)
{
    auto items = listFiles->findItems(fileName, Qt::MatchStartsWith);
    if (!items.isEmpty())
    {
        listFiles->clearSelection();
        listFiles->scrollToItem(items.first());
        listFiles->setCurrentItem(items.first());
    }
}

void ArchiveWindow::onListRowChanged(int idx)
{
    auto selectedSomething = listFiles->selectedItems().count() > 1 || (idx >= 0 && listFiles->item(idx)->text() != "..");
    actionDelete->setEnabled(selectedSomething);

#ifdef WITH_DICOM
    actionStore->setEnabled(selectedSomething && !QSettings().value("dicom/storage-servers").toStringList().isEmpty());
#endif

    stopMedia();

    if (selectedSomething)
    {
        QFileInfo fi(curr.absoluteFilePath(listFiles->item(idx)->text()));
        if (fi.isFile() && actionMode->data().toInt() == GALLERY_MODE && idx >= 0)
        {
            playMediaFile(fi);
        }
    }
}

void ArchiveWindow::onListItemDoubleClicked(QListWidgetItem* item)
{
    QFileInfo fi(curr.absoluteFilePath(item->text()));
    if (fi.isDir())
    {
        setPath(fi.absoluteFilePath());
    }
    else if (actionMode->data().toInt() != GALLERY_MODE)
    {
        playMediaFile(fi);
    }
}

void ArchiveWindow::onListKey()
{
    auto item = listFiles->currentItem();
    if (item)
    {
        onListItemDoubleClicked(item);
    }
}

void ArchiveWindow::switchViewMode(int mode)
{
    auto action = actionMode->menu()->actions().at(mode);

    actionMode->setIcon(action->icon());
    actionMode->setText(action->text());
    actionMode->setData(action->data());
    actionMode->menu()->setDefaultAction(action);

    QSettings().setValue("ui/archive-mode", mode);

    if (mode == GALLERY_MODE)
    {
        listFiles->setViewMode(QListView::IconMode);
        listFiles->setMaximumHeight(176);
        listFiles->setMinimumHeight(144);
        listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        //listFiles->setSelectionMode(QListWidget::SingleSelection);
        listFiles->setWrapping(false);
        player->setVisible(true);
    }
    else
    {
        player->setVisible(false);
        listFiles->setViewMode((QListView::ViewMode)mode);
        listFiles->setMaximumHeight(QWIDGETSIZE_MAX);
        listFiles->setMinimumSize(videoSize);
        listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        //listFiles->setSelectionMode(QListWidget::ExtendedSelection);
        listFiles->setWrapping(true);
    }

    QVariant sizeHint;
    if (mode != QListView::ListMode)
    {
        sizeHint.setValue(QSize(176, 144));
    }

    for (int i = 0; i < listFiles->count(); ++i)
    {
        listFiles->item(i)->setData(Qt::SizeHintRole, sizeHint);
    }
    listFiles->setIconSize(mode == QListView::ListMode? QSize(32, 32): QSize(144, 144));
    listFiles->setVerticalScrollBarPolicy(
        mode == QListView::IconMode? Qt::ScrollBarAsNeeded: Qt::ScrollBarAlwaysOff);
}

void ArchiveWindow::onSwitchModeClick()
{
    auto count = actionMode->menu()->actions().size();
    auto currMode = actionMode->data().toInt();
    switchViewMode(++currMode % count);
}

void ArchiveWindow::onSwitchModeClick(QAction* action)
{
    switchViewMode(action->data().toInt());
}

void ArchiveWindow::onShowFolderClick()
{
    reallyDeleteFiles();
    QDesktopServices::openUrl(QUrl::fromLocalFile(curr.absolutePath()));
}

void static removeFileOrFolder(const QString& path)
{
    if (QFile(path).remove())
    {
        return;
    }

    QDir dir(path);
    foreach (auto file, dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot))
    {
        removeFileOrFolder(dir.absoluteFilePath(file));
    }
    dir.rmdir(path);
}

void ArchiveWindow::reallyDeleteFiles()
{
    if (deleteLater.isEmpty())
    {
        return;
    }

    foreach (auto file, deleteLater)
    {
        removeFileOrFolder(file);
    }

    deleteLater.clear();
    actionRestore->setEnabled(false);
}

void ArchiveWindow::queueFileDeletion(QListWidgetItem* item)
{
    if (item->text() == "..")
    {
        // User definitelly does not want this folder to be removed
        //
        return;
    }

    if (item->isSelected())
    {
        stopMedia();
    }

    // Delete both the clip and the thumbnail
    //
    deleteLater << curr.absoluteFilePath(item->text());
    auto strPreviewFile = item->data(Qt::UserRole).toString();
    if (!strPreviewFile.isEmpty())
    {
        deleteLater << strPreviewFile;
    }

    delete item;
}

void ArchiveWindow::onListItemDraggedOut(QListWidgetItem* item)
{
    reallyDeleteFiles();
    queueFileDeletion(item);
    actionRestore->setEnabled(true);
}

void ArchiveWindow::onDeleteClick()
{
    auto items = listFiles->selectedItems();

    if (items.isEmpty())
    {
        if (!listFiles->currentItem())
        {
            // Nothing is selected. Should never be happend
            //
            return;
        }

        items << listFiles->currentItem();
    }

    listFiles->clearSelection();
    listFiles->setCurrentRow(-1);

    reallyDeleteFiles();

    foreach (auto item, items)
    {
        queueFileDeletion(item);
    }
    actionRestore->setEnabled(true);
}

void ArchiveWindow::onUsbDiskChanged()
{
    killTimer(updateUsbTimerId);
    updateUsbTimerId = startTimer(200);
}

void ArchiveWindow::updateUsbStoreButton()
{
    auto disks = collectRemovableDrives();
    btnUsbStore->setDisabled(disks.empty());

    if (btnUsbStore->isEnabled())
    {
        btnUsbStore->setProperty("disk", disks.first());
        QMenu* menu = nullptr;

        // Omit the menu if the user has nothing to choose
        //
        if (disks.size() > 1)
        {
            menu = new QMenu;
            foreach (auto disk, disks)
            {
                auto diskLabel = QFileInfo(disk).fileName();
                if (diskLabel.isEmpty())
                {
                    diskLabel = disk;
                }
                menu->addAction(QIcon(":/buttons/usb"), diskLabel)->setData(disk);
            }
        }
        btnUsbStore->setMenu(menu);
    }
}

void ArchiveWindow::onUsbStoreMenuClick(QAction* action)
{
    copyToFolder(action->data().toString());
}

void ArchiveWindow::onUsbStoreClick()
{
    copyToFolder(sender()->property("disk").toString());
}

void ArchiveWindow::copyToFolder(const QString& targetPath)
{
    QFileInfoList files = curr.entryInfoList(QDir::Files);

    if (files.empty())
    {
        return;
    }

    auto subDir = root.relativeFilePath(curr.absolutePath());
    QDir targetDir(targetPath);
    targetDir.mkpath(subDir);
    targetDir.cd(subDir);

    QProgressDialog pdlg(this);
    pdlg.setRange(0, files.count());
    pdlg.setMinimumDuration(0);

    bool result = true;
    for (auto i = 0; !pdlg.wasCanceled() && i < files.count(); ++i)
    {
        auto file = files[i];
        auto filePath = file.absoluteFilePath();
        if (QFile::exists(curr.filePath(file.completeBaseName())))
        {
            // Skip clip thumbnail
            //
            continue;
        }

        pdlg.setValue(i);
        pdlg.setLabelText(tr("Copying '%1' to '%2'").arg(file.fileName(), targetDir.absolutePath()));
        qApp->processEvents();
        if (!QFile::copy(filePath, targetDir.absoluteFilePath(file.fileName())))
        {
            auto error = QString::fromLocal8Bit(strerror(errno));
            SetFileExtAttribute(filePath, "usb-status", error);
            if (QMessageBox::Yes != QMessageBox::critical(&pdlg, windowTitle(),
                  tr("Failed to copy '%1' to '%2':\n%3\nContinue?")
                      .arg(file.fileName(), targetDir.absolutePath(), error),
                  QMessageBox::Yes, QMessageBox::No))
            {
                // The user choose to cancel
                //
                result = false;
                break;
            }
        }
        else
        {
            SetFileExtAttribute(filePath, "usb-status", "ok");
        }
    }
    result = result && !pdlg.wasCanceled();
    pdlg.close();

    if (result)
    {
        int userChoice = QMessageBox::information(this, windowTitle(),
            tr("All files were successfully copied."), QMessageBox::Close | QMessageBox::Ok, QMessageBox::Close);
        if (QMessageBox::Close == userChoice)
        {
            onBackToMainWindowClick();
        }
    }
}

#ifdef WITH_DICOM
void ArchiveWindow::onStoreClick()
{
    QWaitCursor wait(this);
    PatientDataDialog dlg(true, "archive-store", this);
    DcmDataset patientDs;
    auto cond = patientDs.loadFile((const char*)curr.absoluteFilePath(".patient.dcm").toLocal8Bit());
    if (cond.good())
    {
        dlg.readPatientData(&patientDs);
    }
    else
    {
        auto localPatientInfoFile = curr.absoluteFilePath(".patient");
        QSettings patientData(localPatientInfoFile, QSettings::IniFormat);
        dlg.readPatientData(patientData);
    }

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    reallyDeleteFiles();
    dlg.savePatientData(&patientDs);
    QFileInfoList files = curr.entryInfoList(QDir::Files);

    DcmClient client;
    if (client.sendToServer(this, &patientDs, files))
    {
        int userChoice = QMessageBox::information(this, windowTitle(),
            tr("All files were successfully stored."), QMessageBox::Close | QMessageBox::Ok, QMessageBox::Close);
        if (QMessageBox::Close == userChoice)
        {
            onBackToMainWindowClick();
        }
    }
}
#endif

void ArchiveWindow::onEditClick()
{
    if (!listFiles->currentItem())
    {
        // Nothing is selected. Should never be happend
        //
        return;
    }

    QSettings settings;
    auto filePath = curr.absoluteFilePath(listFiles->currentItem()->text());
    auto editorExecutable = settings.value("video-editor-app", qApp->applicationFilePath()).toString();
    auto editorSwitches = settings.value("video-editor-switches", QStringList() << "--edit-video").toStringList();
    editorSwitches.append(filePath);

    if (!QProcess::startDetached(editorExecutable, editorSwitches))
    {
        auto err = tr("Failed to launch ").append(editorExecutable);
        foreach (auto arg, editorSwitches)
        {
            err.append(' ').append(arg);
        }
        QMessageBox::critical(this, tr("Archive"), err);
    }
}

void ArchiveWindow::onPrevClick()
{
    if (listFiles->currentRow() > 0)
    {
        listFiles->setCurrentRow(listFiles->currentRow() - 1);
    }
    else
    {
        listFiles->setCurrentRow(listFiles->count() - 1);
    }
}

void ArchiveWindow::onNextClick()
{
    if (listFiles->currentRow() < listFiles->count() - 1)
    {
        listFiles->setCurrentRow(listFiles->currentRow() + 1);
    }
    else
    {
        listFiles->setCurrentRow(0);
    }
}

void ArchiveWindow::onSeekClick()
{
    QGst::PositionQueryPtr queryPos = QGst::PositionQuery::create(QGst::FormatTime);
    pipeline->query(queryPos);

    QGst::DurationQueryPtr queryLen = QGst::DurationQuery::create(QGst::FormatTime);
    pipeline->query(queryLen);

    auto newPos = queryPos->position() + static_cast<QAction*>(sender())->data().toInt();

    if (newPos >= 0 && newPos < queryLen->duration())
    {
        QGst::SeekEventPtr evt = QGst::SeekEvent::create(
             1.0, QGst::FormatTime, QGst::SeekFlagFlush,
             QGst::SeekTypeSet, newPos,
             QGst::SeekTypeNone, QGst::ClockTime::None
         );

        pipeline->sendEvent(evt);
    }
}

void ArchiveWindow::onPlayPauseClick()
{
    pipeline && pipeline->setState(pipeline->currentState() == QGst::StatePlaying? QGst::StatePaused: QGst::StatePlaying);
}

void ArchiveWindow::stopMedia()
{
    foreach (auto action, barMediaControls->actions())
    {
        action->setVisible(false);
    }

    if (pipeline)
    {
        pipeline->setState(QGst::StateNull);
        pipeline->getState(nullptr, nullptr, 10 * GST_SECOND); // 1 sec
        for (int i = 0; i < pagesWidget->count(); ++i)
        {
            static_cast<QGst::Ui::VideoWidget*>(pagesWidget->widget(i))->stopPipelineWatch();
        }
        pipeline.clear();
    }
}

void ArchiveWindow::playMediaFile(const QFileInfo& fi)
{
    if (actionMode->data().toInt() != GALLERY_MODE)
    {
        switchViewMode(GALLERY_MODE);
        return;
        //
        // Will get here again once switchig is done
    }

    auto isVideo = false;
    stopMedia();

    auto caps = TypeDetect(fi.absoluteFilePath());
    if (caps.isEmpty() || caps.startsWith("application/"))
    {
        return;
    }

    try
    {
//      auto pipeDef = QString("filesrc location=\"%1\" ! decodebin ! autovideosink name=displaysink async=0").arg(fi.absoluteFilePath());
//      auto pipeDef = QString("uridecodebin uri=\"%1\" ! autovideosink name=displaysink async=0").arg(QUrl::fromLocalFile(fi.absoluteFilePath()).toString());
        auto pipeDef = QString("playbin2 uri=\"%1\"").arg(QUrl::fromLocalFile(fi.absoluteFilePath()).toEncoded().constData());
        pipeline = QGst::Parse::launch(pipeDef).dynamicCast<QGst::Pipeline>();
        auto hiddenVideoWidget = static_cast<QGst::Ui::VideoWidget*>(pagesWidget->widget(1 - pagesWidget->currentIndex()));
        hiddenVideoWidget->watchPipeline(pipeline);
        QGlib::connect(pipeline->bus(), "message", this, &ArchiveWindow::onBusMessage);
        pipeline->bus()->addSignalWatch();
        pipeline->setState(QGst::StatePaused);
        pipeline->getState(nullptr, nullptr, GST_SECOND * 10); // 10 sec
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().append(".archive").toUtf8());
        isVideo = caps.startsWith("video/");
    }
    catch (const QGlib::Error& ex)
    {
        const QString msg = ex.message();
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
    }

    actionEdit->setEnabled(isVideo);
    foreach (auto action, barMediaControls->actions())
    {
        action->setVisible(isVideo);
    }
}

void ArchiveWindow::onBusMessage(const QGst::MessagePtr& message)
{
    //qDebug() << message->typeName() << " " << message->source()->property("name").toString();

    switch (message->type())
    {
    case QGst::MessageStateChanged:
        onStateChangedMessage(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageElement:
        {
            const QGst::StructurePtr s = message->internalStructure();
            if (!s)
            {
                qDebug() << "Got empty QGst::MessageElement";
            }
            else if (s->name() == "prepare-xwindow-id" || s->name() == "prepare-window-handle")
            {
                // At this time the video output finally has a sink, so set it up now
                //
                message->source()->setProperty("force-aspect-ratio", true);
            }
        }
        break;
    case QGst::MessageError:
        {
            auto ex = message.staticCast<QGst::ErrorMessage>()->error();
            auto obj = message->source();
            QString msg;
            if (obj)
            {
                msg.append(obj->property("name").toString()).append(' ');
            }
            msg.append(ex.message());

            qCritical() << msg;
#ifndef Q_OS_WIN
            // Showing a message box under Microsoft (R) Windows (TM) breaks everything,
            // since it becomes the active one and the video output goes here.
            // So we can no more then hide this error from the user.
            //
            QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
#endif
        }
      break;
    case QGst::MessageEos:
        {
            // Rewind to the start and pause the video
            //
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                 1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                 QGst::SeekTypeSet, 0,
                 QGst::SeekTypeNone, QGst::ClockTime::None
             );

            pipeline->sendEvent(evt);
            pipeline->setState(QGst::StatePaused);
        }
        break;
#ifdef QT_DEBUG
    case QGst::MessageDuration:
        {
            auto msg = message.staticCast<QGst::DurationMessage>();
            qDebug() << "Duration" << msg->format() << msg->duration();
        }
        break;
    case QGst::MessageInfo:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageAsyncDone:
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageTag:
        break;
    default:
        qDebug() << message->type();
        break;
#else
    default: // Make the compiler happy
        break;
#endif
    }
}

void ArchiveWindow::onStateChangedMessage(const QGst::StateChangedMessagePtr& message)
{
    //qDebug() << message->oldState() << " => " << message->newState();

    if (message->source() == pipeline)
    {
        if (message->oldState() == QGst::StateReady && message->newState() == QGst::StatePaused)
        {
            // The aspect ratio has been already fixed, time to show the video
            //
            pagesWidget->setCurrentIndex(1 - pagesWidget->currentIndex());

            // Time to adjust framerate
            //
            gint numerator = 0, denominator = 0;
            auto sink = pipeline->getElementByName("displaysink");
            if (sink)
            {
                auto pad = sink->getStaticPad("sink");
                if (pad)
                {
                    auto caps = pad->negotiatedCaps();
                    if (caps)
                    {
                        auto s = caps->internalStructure(0);
                        gst_structure_get_fraction (s.data()->operator const GstStructure *(), "framerate", &numerator, &denominator);
                    }
                }
            }

            if (denominator > 0 && numerator > 0)
            {
                auto frameDuration = (GST_SECOND * denominator) / numerator + 1;
                //qDebug() << "Framerate " << denominator << "/" << numerator << " duration" << frameDuration;
                actionSeekFwd->setData((int)frameDuration);
                actionSeekBack->setData((int)-frameDuration);
            }

            // The pipeline now in paused state.
            // At this time we still don't know, is it a clip or a picture.
            // So, seek a bit forward to figure it out.
            //
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                 1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                 QGst::SeekTypeCur, 0,
                 QGst::SeekTypeNone, QGst::ClockTime::None
             );

            pipeline->sendEvent(evt);
        }

        // Make sure the play/pause button is in consistent state
        //
        actionPlay->setIcon(QIcon(message->newState() == QGst::StatePlaying? ":buttons/pause": ":buttons/play"));
        actionPlay->setText(message->newState() == QGst::StatePlaying? tr("Pause"): tr("Play"));
    }
}

void ArchiveWindow::onBackToMainWindowClick()
{
    reallyDeleteFiles();
    auto stackWidget = static_cast<SlidingStackedWidget*>(parent()->qt_metacast("SlidingStackedWidget"));
    if (stackWidget)
    {
        stackWidget->slideInWidget("Main");
    }
}
