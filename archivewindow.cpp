/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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
#include "qwaitcursor.h"

#ifdef WITH_DICOM
#include "dicom/dcmclient.h"
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcuid.h>
#endif

#ifdef WITH_TOUCH
#include "touch/slidingstackedwidget.h"
#endif

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileIconProvider>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QSlider>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

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

#if defined(UNICODE) || defined (_UNICODE)
#include <MediaInfo/MediaInfo.h>
#else
#define UNICODE
#include <MediaInfo/MediaInfo.h>
#undef UNICODE
#endif

#define GALLERY_MODE 2

static QSize videoSize(352, 258);

static void DamnQtMadeMeDoTheSunsetByHands(QToolBar* bar)
{
    foreach (auto action, bar->actions())
    {
        auto shortcut = action->shortcut();
        if (shortcut.isEmpty())
        {
            continue;
        }
        action->setToolTip(action->text() + " (" + shortcut.toString(QKeySequence::NativeText) + ")");
    }
}

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QWidget(parent)
{
    auto layoutMain = new QVBoxLayout;

    auto barArchive = new QToolBar(tr("Archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
#ifdef WITH_TOUCH
    auto actionBack = barArchive->addAction(QIcon(":buttons/back"), tr("Back"), this, SLOT(onBackToMainWindowClick()));
    actionBack->setShortcut(Qt::Key_Back);
#endif
    actionDelete = barArchive->addAction(QIcon(":buttons/delete"), tr("Delete"), this, SLOT(onDeleteClick()));
    actionDelete->setShortcut(Qt::Key_Delete);
    actionDelete->setEnabled(false);

#ifdef WITH_DICOM
    actionStore = barArchive->addAction(QIcon(":buttons/dicom"), tr("Dicom"), this, SLOT(onStoreClick()));
    actionStore->setShortcut(Qt::Key_F5);
    actionStore->setEnabled(false);
#endif

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barArchive->addWidget(spacer);

    actionMode = new QAction(barArchive);
    actionMode->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_0));
    connect(actionMode, SIGNAL(triggered()), this, SLOT(onSwitchModeClick()));

    auto menuMode = new QMenu;
    connect(menuMode, SIGNAL(triggered(QAction*)), this, SLOT(onSwitchModeClick(QAction*)));
    auto actionListMode = menuMode->addAction(QIcon(":buttons/list"), tr("List"));
    actionListMode->setData(QListView::ListMode);
    actionListMode->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_1));
    auto actionIconMode = menuMode->addAction(QIcon(":buttons/icons"), tr("Icons"));
    actionIconMode->setData(QListView::IconMode);
    actionIconMode->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_2));
    auto actionGalleryMode = menuMode->addAction(QIcon(":buttons/gallery"), tr("Gallery"));
    actionGalleryMode->setData(GALLERY_MODE);
    actionGalleryMode->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_3));

    actionMode->setMenu(menuMode);
    barArchive->addAction(actionMode);

    auto actionBrowse = barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));
    actionBrowse->setShortcut(Qt::Key_F2); // Same as the key that open this dialog

#ifndef WITH_TOUCH
    layoutMain->addWidget(barArchive);
#endif

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
    btnPrev->setToolTip(btnPrev->text() + " (" + QKeySequence(Qt::Key_Right).toString(QKeySequence::NativeText) + ")");
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
    btnNext->setToolTip(btnNext->text() + " (" + QKeySequence(Qt::Key_Left).toString(QKeySequence::NativeText) + ")");
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
    actionSeekBack->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Left));
    actionPlay = barMediaControls->addAction(QIcon(":buttons/record"), tr("Play"), this, SLOT(onPlayPauseClick()));
    actionPlay->setVisible(false);
    actionPlay->setShortcut(QKeySequence(Qt::Key_Space));
    actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  tr("Forward"), this, SLOT(onSeekClick()));
    actionSeekFwd->setVisible(false);
    actionSeekFwd->setData(+40000000);
    actionSeekFwd->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Right));
    barMediaControls->setMinimumSize(48, 48);

    playerLayout->addWidget(barMediaControls, 0, Qt::AlignHCenter);

    player->setLayout(playerLayout);
    layoutMain->addWidget(player);

    listFiles = new QListWidget;
//    listFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
//    listFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listFiles->setMovement(QListView::Static);
    connect(listFiles, SIGNAL(currentRowChanged(int)), this, SLOT(onListRowChanged(int)));
    connect(listFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onListItemDoubleClicked(QListWidgetItem*)));
    auto actionEnter = new QAction(listFiles);
    actionEnter->setShortcut(Qt::Key_Return);
    connect(actionEnter, SIGNAL(triggered()), this, SLOT(onListKey()));
    listFiles->addAction(actionEnter);
    layoutMain->addWidget(listFiles);

#ifdef WITH_TOUCH
    layoutMain->addWidget(barArchive);
#endif
    setLayout(layoutMain);

    QSettings settings;
    restoreGeometry(settings.value("archive-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("archive-state").toInt());

    setAttribute(Qt::WA_DeleteOnClose, false);

    DamnQtMadeMeDoTheSunsetByHands(barArchive);
    DamnQtMadeMeDoTheSunsetByHands(barMediaControls);
}

ArchiveWindow::~ArchiveWindow()
{
    stopMedia();
}

void ArchiveWindow::closeEvent(QCloseEvent *evt)
{
    QSettings settings;
    settings.setValue("archive-geometry", saveGeometry());
    settings.setValue("archive-state", (int)windowState() & ~Qt::WindowMinimized);
    QWidget::closeEvent(evt);
}

void ArchiveWindow::updateRoot()
{
    QSettings settings;
    root.setPath(settings.value("output-path", "/video").toString());
    curr = root;
    switchViewMode(settings.value("archive-mode").toInt());
}

void ArchiveWindow::setPath(const QString& path)
{
    curr.setPath(path);
    QTimer::singleShot(0, this, SLOT(updatePath()));
}

void ArchiveWindow::createSubDirMenu(QAction* parentAction)
{
    QDir dir(parentAction->data().toString());
    auto menu = new QMenu;

    foreach (auto subDir, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
    {
        menu->addAction(subDir.baseName(), this, SLOT(selectPath(bool)))->setData(subDir.absoluteFilePath());
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

    updateList();
    listFiles->setCurrentRow(0);
}

void ArchiveWindow::preparePathPopupMenu()
{
    auto menu = static_cast<QMenu*>(sender());
    foreach (auto action, menu->actions())
    {
        createSubDirMenu(action);
    }
}

void ArchiveWindow::updateList()
{
    QWaitCursor wait(this);
    MediaInfoLib::MediaInfo mi;
    QFileIconProvider fip;

    listFiles->setUpdatesEnabled(false);
    listFiles->clear();
    auto filter = QDir::NoDot | QDir::AllEntries;

    // No "parent folder" item in gallery mode and on the root
    //
    if (curr == root || actionMode->data().toInt() == GALLERY_MODE)
    {
        filter |= QDir::NoDotDot;
    }

    foreach (QFileInfo fi, curr.entryInfoList(filter))
    {
        QIcon icon;
        QPixmap pm;

        if (fi.isDir())
        {
            if (fi.fileName() == "..")
            {
                icon.addFile(":/buttons/up");
            }
            else
            {
                icon.addFile(":/buttons/folder");
            }
        }
        else if (pm.load(fi.absoluteFilePath()))
        {
            if (QFile::exists(curr.absoluteFilePath(fi.completeBaseName())))
            {
                auto clip = listFiles->findItems(fi.completeBaseName(), Qt::MatchExactly);
                if (!clip.isEmpty())
                {
                    // Got a snapshot for a clip file. Add a fency overlay to it
                    //
                    QPixmap pmOverlay(":/buttons/film");
                    QPainter painter(&pm);
                    painter.setOpacity(0.75);
                    painter.drawPixmap(pm.rect(), pmOverlay);
                    icon.addPixmap(pm);
                    clip.first()->setIcon(icon);
                    clip.first()->setData(Qt::UserRole, fi.absoluteFilePath());
                }
                continue;
            }
            icon.addPixmap(pm);
        }
        else
        {
            if (mi.Open(fi.absoluteFilePath().toStdWString())
                && QString::fromStdWString(mi.Get(MediaInfoLib::Stream_General, 0, __T("VideoCount"))) != "0")
            {
                icon.addFile(":/buttons/movie");
            }
            else
            {
                icon = fip.icon(fi); // Should never be happen.
            }
        }

        auto item = new QListWidgetItem(icon, fi.fileName(), listFiles);
        item->setToolTip(fi.absoluteFilePath());
    }

    listFiles->setUpdatesEnabled(true);
}

void ArchiveWindow::selectPath(QAction* action)
{
    setPath(action->data().toString());
}

void ArchiveWindow::selectPath(bool)
{
    selectPath(static_cast<QAction*>(sender()));
}

void ArchiveWindow::onListRowChanged(int idx)
{
    auto selectedSomething = listFiles->selectedItems().count() > 1 || (idx >= 0 && listFiles->item(idx)->text() != "..");
    actionDelete->setEnabled(selectedSomething);
#ifdef WITH_DICOM
    actionStore->setEnabled(selectedSomething && QFile::exists(curr.absoluteFilePath(".patient.dcm"))
                            && !QSettings().value("storage-servers").toStringList().isEmpty());
#endif

    if (actionMode->data().toInt() == GALLERY_MODE && idx >= 0)
    {
        QFileInfo fi(curr.absoluteFilePath(listFiles->item(idx)->text()));
        if (!fi.isDir())
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
    else
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

    QSettings().setValue("archive-mode", mode);

    if (mode == GALLERY_MODE)
    {
        listFiles->setViewMode(QListView::IconMode);
        listFiles->setMaximumHeight(176);
        listFiles->setMinimumHeight(144);
        listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        listFiles->setSelectionMode(QListWidget::SingleSelection);
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
        listFiles->setSelectionMode(QListWidget::ExtendedSelection);
        listFiles->setWrapping(true);
    }

    listFiles->setIconSize(mode == QListView::ListMode? QSize(32, 32): QSize(144, 144));

    updateList();
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

    int userChoice = QMessageBox::question(this, windowTitle(),
        items.count() == 1? tr("Are you sure to delete\n\n'%1'?").arg(items.first()->text()): tr("Are you sure to delete selected items?"),
        QMessageBox::Ok, QMessageBox::Cancel | QMessageBox::Default);

    if (QMessageBox::Ok == userChoice)
    {
        stopMedia();
        QWaitCursor wait(this);

        foreach (auto item, items)
        {
            if (item->text() == "..")
            {
                // User definitelly does not want this folder to be removed
                //
                continue;
            }

            removeFileOrFolder(curr.absoluteFilePath(item->text()));

            // Delete both the clip and the thumbnail
            //
            auto strPreviewFile = item->data(Qt::UserRole).toString();
            if (!strPreviewFile.isEmpty())
            {
                QFile(strPreviewFile).remove();
            }
            delete item;
        }
    }
}

#ifdef WITH_DICOM
void ArchiveWindow::onStoreClick()
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

    DcmDataset patientDs;
    auto cond = patientDs.loadFile((const char*)curr.absoluteFilePath(".patient.dcm").toLocal8Bit());
    if (cond.bad())
    {
        QMessageBox::critical(this, windowTitle(), tr("Failed to load patient dataset:\n\n%1").arg(QString::fromLocal8Bit(cond.text())));
        return;
    }

    int userChoice = QMessageBox::question(this, windowTitle(),
        items.count() == 1? tr("Are you sure to send '%1' to storage servers?").arg(items.first()->text()): tr("Are you sure to send selected items to storage servers?"),
        QMessageBox::Ok, QMessageBox::Cancel | QMessageBox::Default);

    if (QMessageBox::Ok == userChoice)
    {
        QFileInfoList files;
        QWaitCursor wait(this);

        foreach (auto item, items)
        {
            if (item->text() == "..")
            {
                // User definitelly does not want this folder to be sent
                //
                continue;
            }

            files << QFileInfo(curr, item->text());
        }

        DcmClient client; // UID_SecondaryCaptureImageStorage | UID_VideoEndoscopicImageStorage | UID_RawDataStorage
        char seriesUID[100] = {0};
        dcmGenerateUniqueIdentifier(seriesUID, SITE_SERIES_UID_ROOT);
        client.sendToServer(this, &patientDs, files, seriesUID);
    }
}
#endif

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
    pipeline->setState(pipeline->currentState() == QGst::StatePlaying? QGst::StatePaused: QGst::StatePlaying);
}

void ArchiveWindow::stopMedia()
{
    setWindowTitle(tr("Archive"));

    if (pipeline)
    {
        for (int i = 0; i < pagesWidget->count(); ++i)
        {
            static_cast<QGst::Ui::VideoWidget*>(pagesWidget->widget(i))->stopPipelineWatch();
        }
        pipeline->setState(QGst::StateNull);
        pipeline->getState(nullptr, nullptr, 1 * GST_SECOND); // 1 sec
        pipeline.clear();
        qApp->processEvents();
    }
}

void ArchiveWindow::playMediaFile(const QFileInfo& fi)
{
    if (actionMode->data().toInt() != GALLERY_MODE)
    {
        switchViewMode(GALLERY_MODE);
        listFiles->setCurrentItem(listFiles->findItems(fi.fileName(), Qt::MatchExactly).first());
        return;
        //
        // Will get here again once switchig is done
    }

    stopMedia();

    try
    {
//      auto pipeDef = QString("filesrc location=\"%1\" ! decodebin ! autovideosink name=displaysink async=0").arg(fi.absoluteFilePath());
        auto pipeDef = QString("uridecodebin uri=\"%1\" ! autovideosink name=displaysink async=0").arg(QUrl::fromLocalFile(fi.absoluteFilePath()).toString());
        pipeline = QGst::Parse::launch(pipeDef).dynamicCast<QGst::Pipeline>();
        auto hiddenVideoWidget = static_cast<QGst::Ui::VideoWidget*>(pagesWidget->widget(1 - pagesWidget->currentIndex()));
        hiddenVideoWidget->watchPipeline(pipeline);
        QGlib::connect(pipeline->bus(), "message", this, &ArchiveWindow::onBusMessage);
        pipeline->bus()->addSignalWatch();
        pipeline->setState(QGst::StatePaused);
        pipeline->getState(nullptr, nullptr, GST_SECOND * 10); // 10 sec
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().toUtf8());
        setWindowTitle(tr("Archive - %1").arg(fi.fileName()));
    }
    catch (QGlib::Error ex)
    {
        const QString msg = ex.message();
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
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
#ifndef Q_WS_WIN
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
    case QGst::MessageAsyncDone:
        {
            // At this time we finally is able to query is the pipeline seekable or not.
            //
            QGst::SeekingQueryPtr querySeek = QGst::SeekingQuery::create(QGst::FormatTime);
            pipeline->query(querySeek);
            //qDebug() << " seekable " << querySeek->seekable() << " format " << querySeek->format();
            auto isClip = querySeek->seekable();
            foreach (auto action, barMediaControls->actions())
            {
                action->setVisible(isClip);
            }
        }
        break;
#ifdef QT_DEBUG
    case QGst::MessageInfo:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
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
        if (message->oldState() == QGst::StateNull && message->newState() == QGst::StateReady)
        {
            auto sink = pipeline->getElementByName("displaysink");
            if (sink)
            {
                auto childProxy =  sink.dynamicCast<QGst::ChildProxy>();
                if (childProxy)
                {
                    auto cnt = childProxy->childrenCount();
                    for (uint i = 0; i < cnt; ++i)
                    {
                        auto child = childProxy->childByIndex(i);
                        child->setProperty("force-aspect-ratio", true);
                    }
                }
                else
                {
                    sink->setProperty("force-aspect-ratio", true);
                }
            }
        }
        else if (message->oldState() == QGst::StateReady && message->newState() == QGst::StatePaused)
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
        actionPlay->setIcon(QIcon(message->newState() == QGst::StatePlaying? ":buttons/pause": ":buttons/record"));
        actionPlay->setText(message->newState() == QGst::StatePlaying? tr("Pause"): tr("Play"));
    }
}

#ifdef WITH_TOUCH
void ArchiveWindow::onBackToMainWindowClick()
{
    auto stackWidget = static_cast<SlidingStackedWidget*>(parent()->qt_metacast("SlidingStackedWidget"));
    if (stackWidget)
    {
        stackWidget->slideInIdx(stackWidget->property("MainWidget").toInt());
    }
}
#endif
