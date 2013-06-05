#include "archivewindow.h"
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
#include <QSlider>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/Event>
#include <QGst/Parse>
#include <QGst/Query>

#include <gst/gstdebugutils.h>

static QSize videoSize(352, 258);

static void DamnQtMadeMeDoTheSunsetByHands(QToolBar* bar)
{
    Q_FOREACH(auto action, bar->actions())
    {
        auto shortcut = action->shortcut();
        if (!shortcut)
        {
            continue;
        }
        action->setToolTip(action->text() + " (" + shortcut.toString(QKeySequence::NativeText) + ")");
    }
}

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QDialog(parent)
{
    QBoxLayout* layoutMain = new QVBoxLayout;

    QToolBar* barArchive = new QToolBar(tr("Archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    actionDelete = barArchive->addAction(QIcon(":buttons/delete"), tr("Delete"), this, SLOT(onDeleteClick()));
    actionDelete->setShortcut(Qt::Key_Delete);
    actionDelete->setEnabled(false);

    auto actionBrowse = barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));
    actionBrowse->setShortcut(Qt::Key_F2); // Same as the key that open this dialog

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
    actionGalleryMode->setData(2);
    actionGalleryMode->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_3));

    actionMode->setMenu(menuMode);
    barArchive->addAction(actionMode);

    layoutMain->addWidget(barArchive);

    barPath = new QToolBar(tr("Path"));
    layoutMain->addWidget(barPath);

    player = new QWidget;

    QBoxLayout* playerLayout = new QVBoxLayout();
    playerLayout->setContentsMargins(0,16,0,16);

    QBoxLayout* playerInnerLayout = new QHBoxLayout();
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

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(videoSize);
    displayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playerInnerLayout->addWidget(displayWidget);

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
    auto actionSeekBack = barMediaControls->addAction(QIcon(":buttons/rewind"), tr("Rewing"), this, SLOT(onSeekClick()));
    actionSeekBack->setVisible(false);
    actionSeekBack->setData(-500000000);
    actionSeekBack->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Left));
    actionPlay = barMediaControls->addAction(QIcon(":buttons/record"), tr("Play"), this, SLOT(onPlayPauseClick()));
    actionPlay->setVisible(false);
    actionPlay->setShortcut(QKeySequence(Qt::Key_Space));
    auto actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  tr("Forward"), this, SLOT(onSeekClick()));
    actionSeekFwd->setVisible(false);
    actionSeekFwd->setData(+500000000);
    actionSeekFwd->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Right));
    barMediaControls->setMinimumSize(48, 48);

    playerLayout->addWidget(barMediaControls, 0, Qt::AlignHCenter);

    player->setLayout(playerLayout);
    layoutMain->addWidget(player);

    listFiles = new QListWidget;
    listFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    listFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listFiles->setMovement(QListView::Static);
    listFiles->setWrapping(false);
    connect(listFiles, SIGNAL(currentRowChanged(int)), this, SLOT(onListRowChanged(int)));
    connect(listFiles, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onListItemDoubleClicked(QListWidgetItem*)));
    auto actionEnter = new QAction(listFiles);
    actionEnter->setShortcut(Qt::Key_Return);
    connect(actionEnter, SIGNAL(triggered()), this, SLOT(onListKey()));
    listFiles->addAction(actionEnter);
    layoutMain->addWidget(listFiles);

    setLayout(layoutMain);

    QSettings settings;
    restoreGeometry(settings.value("archive-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("archive-state").toInt());

    setAttribute(Qt::WA_DeleteOnClose, false);
    switchViewMode(settings.value("archive-mode").toInt());

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

void ArchiveWindow::setRoot(const QString& path)
{
    root.setPath(path);
}

void ArchiveWindow::setPath(const QString& path)
{
    curr.setPath(path);
    updatePath();
    listFiles->setCurrentRow(0);
}

void ArchiveWindow::createSubDirMenu(QAction* parentAction)
{
    QDir dir(parentAction->data().toString());
    QMenu* menu = new QMenu;

    Q_FOREACH(auto subDir, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
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
        createSubDirMenu(action);
        barPath->insertAction(prev, action);
        prev = action;
    } while (dir != root && dir.cdUp());

    updateList();
}

void ArchiveWindow::preparePathPopupMenu()
{
    auto menu = static_cast<QMenu*>(sender());
    Q_FOREACH(auto action, menu->actions())
    {
        createSubDirMenu(action);
    }
}

void ArchiveWindow::updateList()
{
    QFileIconProvider fip;

    listFiles->setUpdatesEnabled(false);
    listFiles->clear();
    QFlags<QDir::Filter> filter = QDir::NoDot | QDir::AllEntries;

    // No "parent folder" item in gallery mode and on the root
    //
    if (curr == root || actionMode->data().toInt() == 2)
    {
        filter |= QDir::NoDotDot;
    }

    Q_FOREACH(QFileInfo fi, curr.entryInfoList(filter))
    {
        QIcon icon;
        QPixmap pm;

        if (fi.fileName() == "..")
        {
            icon.addFile(":/buttons/up");
        }
        else if (!fi.isDir() && pm.load(fi.absoluteFilePath()))
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
                    clip.at(0)->setIcon(icon);
                    clip.at(0)->setData(Qt::UserRole, fi.absoluteFilePath());
                }
                continue;
            }
            icon.addPixmap(pm);
        }
        else
        {
            icon = fip.icon(fi);
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
    actionDelete->setEnabled(idx >= 0);
    if (actionMode->data().toInt() == 2 && idx >= 0)
    {
        playMediaFile(curr.absoluteFilePath(listFiles->item(idx)->text()));
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
        playMediaFile(fi.absoluteFilePath());
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

    if (mode == 2)
    {
        barPath->setVisible(false);
        listFiles->setViewMode(QListView::IconMode);
        listFiles->setMaximumHeight(160);
        listFiles->setMinimumHeight(144);
        listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        player->setVisible(true);
    }
    else
    {
        player->setVisible(false);
        listFiles->setViewMode((QListView::ViewMode)mode);
        listFiles->setMaximumHeight(QWIDGETSIZE_MAX);
        listFiles->setMinimumSize(videoSize);
        listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        barPath->setVisible(true);
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
    QDesktopServices::openUrl(QUrl(curr.absolutePath()));
}

void static removeFileOrFolder(const QString& path)
{
    if (QFile(path).remove())
    {
        return;
    }

    QDir dir(path);
    Q_FOREACH(auto file, dir.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot))
    {
        removeFileOrFolder(dir.absoluteFilePath(file));
    }
    dir.rmdir(path);
}

void ArchiveWindow::onDeleteClick()
{
    auto item = listFiles->currentItem();
    int userChoice = QMessageBox::question(this, windowTitle(),
          tr("Are you sure to delete %1?").arg(curr.absoluteFilePath(item->text())),
          QMessageBox::Ok, QMessageBox::Cancel | QMessageBox::Default);

    if (QMessageBox::Ok == userChoice)
    {
        stopMedia();
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

void ArchiveWindow::onPrevClick()
{
    if (listFiles->currentRow() > 0)
    {
        listFiles->setCurrentRow(listFiles->currentRow() - 1);
    }
}

void ArchiveWindow::onNextClick()
{
    if (listFiles->currentRow() < listFiles->count() - 1)
    {
        listFiles->setCurrentRow(listFiles->currentRow() + 1);
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
        pipeline->setState(QGst::StateNull);
        pipeline->getState(nullptr, nullptr, 1000000000L); // 1 sec
        displayWidget->stopPipelineWatch();
        pipeline.clear();
    }
}

void ArchiveWindow::playMediaFile(const QString& file)
{
    if (actionMode->data().toInt() != 2)
    {
        switchViewMode(2);
    }

    stopMedia();

    try
    {
        pipeline = QGst::Parse::launch(QString("filesrc location=\"%1\" ! decodebin ! autovideosink").arg(file)).dynamicCast<QGst::Pipeline>();
        displayWidget->watchPipeline(pipeline);
        QGlib::connect(pipeline->bus(), "message", this, &ArchiveWindow::onBusMessage);
        pipeline->bus()->addSignalWatch();
        pipeline->setState(QGst::StateReady);
        pipeline->getState(nullptr, nullptr, 1000000000L); // 1 sec
        auto details = GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pipeline.staticCast<QGst::Bin>(), details, qApp->applicationName().toUtf8());
        pipeline->setState(QGst::StatePaused);
        setWindowTitle(tr("Archive - %1").arg(file));
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
            QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
        }
        break;
#ifdef QT_DEBUG
    case QGst::MessageInfo:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::InfoMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message->source()->property("name").toString() << " " << message.staticCast<QGst::WarningMessage>()->error();
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
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
        {
            // At this time we finally is able to query is the pipeline seekable or not.
            //
            QGst::SeekingQueryPtr querySeek = QGst::SeekingQuery::create(QGst::FormatTime);
            pipeline->query(querySeek);
            qDebug() << " seekable " << querySeek->seekable() << " format " << querySeek->format();
            auto isClip = querySeek->seekable();
            Q_FOREACH(auto action, barMediaControls->actions())
            {
                action->setVisible(isClip);
            }
        }
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
