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

static QWidget* createSpacer()
{
    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return spacer;
}

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QDialog(parent)
{
    QBoxLayout* layoutMain = new QVBoxLayout;

    QToolBar* barArchive = new QToolBar(tr("archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    actionDelete = barArchive->addAction(QIcon(":buttons/delete"), tr("Delete"), this, SLOT(onDeleteClick()));
    actionDelete->setEnabled(false);

    barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));

    barArchive->addWidget(createSpacer());

    QActionGroup* group = new QActionGroup(barArchive);
    auto actionList = barArchive->addAction(QIcon(":buttons/list"), tr("List"), this, SLOT(onListClick()));
    actionList->setCheckable(true);
    group->addAction(actionList);
    auto actionGallery = barArchive->addAction(QIcon(":buttons/gallery"), tr("Gallery"), this, SLOT(onGalleryClick()));
    actionGallery->setCheckable(true);
    group->addAction(actionGallery);
    layoutMain->addWidget(barArchive);

    barPath = new QToolBar(tr("path"));
    layoutMain->addWidget(barPath);

    player = new QWidget;

    QBoxLayout* playerLayout = new QVBoxLayout();
    playerLayout->setContentsMargins(0,16,0,16);

    QBoxLayout* playerInnerLayout = new QHBoxLayout();
    playerInnerLayout->setContentsMargins(0,0,0,0);

    auto barPrev = new QToolBar(tr("prev"));
    auto btnPrev = new QToolButton;
    btnPrev->setIcon(QIcon(":buttons/prev"));
    btnPrev->setMinimumHeight(200);
    btnPrev->setFocusPolicy(Qt::NoFocus);
    connect(btnPrev, SIGNAL(clicked()), this, SLOT(onPrevClick()));
    barPrev->addWidget(btnPrev);
    playerInnerLayout->addWidget(barPrev);

    displayWidget = new QGst::Ui::VideoWidget();
    displayWidget->setMinimumSize(videoSize);
    displayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playerInnerLayout->addWidget(displayWidget);

    auto barNext = new QToolBar(tr("next"));
    auto btnNext = new QToolButton;
    btnNext->setIcon(QIcon(":buttons/next"));
    btnNext->setMinimumHeight(200);
    btnNext->setFocusPolicy(Qt::NoFocus);
    connect(btnNext, SIGNAL(clicked()), this, SLOT(onNextClick()));
    barNext->addWidget(btnNext);
    playerInnerLayout->addWidget(barNext);

    playerLayout->addLayout(playerInnerLayout);

    barMediaControls = new QToolBar(tr("media"));
    auto actionSeekBack = barMediaControls->addAction(QIcon(":buttons/rewind"), nullptr, this, SLOT(onSeekClick()));
    actionSeekBack->setVisible(false);
    actionSeekBack->setData(-500000000);
    auto actionPlay = barMediaControls->addAction(QIcon(":buttons/record"), nullptr, this, SLOT(onPlayPauseClick()));
    actionPlay->setVisible(false);
    auto actionSeekFwd = barMediaControls->addAction(QIcon(":buttons/forward"),  nullptr, this, SLOT(onSeekClick()));
    actionSeekFwd->setVisible(false);
    actionSeekFwd->setData(+500000000);
    barMediaControls->setMinimumSize(48, 48);

    playerLayout->addWidget(barMediaControls, 0, Qt::AlignHCenter);

    player->setLayout(playerLayout);
    layoutMain->addWidget(player);

    listFiles = new QListWidget;
    listFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    listFiles->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listFiles->setMovement(QListView::Static);
    listFiles->setWrapping(false);
    connect(listFiles, SIGNAL(currentTextChanged(QString)), this, SLOT(listItemSelected(QString)));
    layoutMain->addWidget(listFiles);

    setLayout(layoutMain);
    setAttribute(Qt::WA_DeleteOnClose, false);
    if (QSettings().value("show-gallery").toBool())
    {
        actionGallery->setChecked(true);
        onGalleryClick();
    }
    else
    {
        actionList->setChecked(true);
        onListClick();
    }
}

ArchiveWindow::~ArchiveWindow()
{
    stopMedia();
}

void ArchiveWindow::setRoot(const QString& path)
{
    root.setPath(path);
}

void ArchiveWindow::setPath(const QString& path)
{
    curr.setPath(path);
    updatePath();
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

    listFiles->clearFocus();
    listFiles->clear();
    listFiles->setUpdatesEnabled(false);
    QFlags<QDir::Filter> filter = QDir::NoDot | QDir::AllEntries;

    // No "parent folder" item in gallery mode and on the root
    //
    if (curr == root || player->isVisible())
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

    if (player->isVisible())
    {
        listFiles->setCurrentRow(0);
    }
}

void ArchiveWindow::selectPath(QAction* action)
{
    setPath(action->data().toString());
}

void ArchiveWindow::selectPath(bool)
{
    selectPath(static_cast<QAction*>(sender()));
}

void ArchiveWindow::listItemSelected(QString item)
{
    if (item.isEmpty())
    {
        actionDelete->setEnabled(false);
        return;
    }

    QFileInfo fi(curr.absoluteFilePath(item));
    if (fi.isDir())
    {
        actionDelete->setEnabled(fi.fileName() != "..");
        setPath(fi.absoluteFilePath());
    }
    else
    {
        actionDelete->setEnabled(true);
        playMediaFile(fi.absoluteFilePath());
    }
}

void ArchiveWindow::onListClick()
{
    QSettings().setValue("show-gallery", false);
    player->setVisible(false);
    listFiles->setViewMode(QListView::ListMode);
    listFiles->setIconSize(QSize(32, 32));
    listFiles->setMaximumHeight(QWIDGETSIZE_MAX);
    listFiles->setMinimumSize(videoSize);
    listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    barPath->setVisible(true);
    updateList();
}

void ArchiveWindow::onGalleryClick()
{
    QSettings().setValue("show-gallery", true);
    barPath->setVisible(false);
    listFiles->setViewMode(QListView::IconMode);
    listFiles->setIconSize(QSize(144, 144));
    listFiles->setMaximumHeight(160);
    listFiles->setMinimumHeight(144);
    listFiles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    player->setVisible(true);
    updateList();
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

    QGst::SegmentQueryPtr querySeg = QGst::SegmentQuery::create(QGst::FormatTime);
    pipeline->query(querySeg);

    qDebug() << "rate " << querySeg->rate() << "type " << querySeg->format()
             << "start " << querySeg->startValue() << "stop " << querySeg->stopValue();

    auto delta = static_cast<QAction*>(sender())->data().toInt();
    auto newPos = queryPos->position() + delta;

    qDebug() << "POS " << queryPos->position() << "type " << queryPos->format()
             << "LEN " << queryLen->duration() << "type " << queryLen->format() << " new " << newPos;

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
    stopMedia();

    if (!player->isVisible())
    {
        return;
    }

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

void ArchiveWindow::setPosition(int value)
{
    QGst::DurationQueryPtr queryLen = QGst::DurationQuery::create(QGst::FormatTime);
    pipeline->query(queryLen);

    uint length = -QGst::ClockTime(queryLen->duration()).toTime().msecsTo(QTime());
    if (length != 0 && value > 0)
    {
        QTime pos;
        pos = pos.addMSecs(length * (value / 1000.0));

        QGst::SeekEventPtr evt = QGst::SeekEvent::create(
             1.0, QGst::FormatTime, QGst::SeekFlagFlush,
             QGst::SeekTypeSet, QGst::ClockTime::fromTime(pos),
             QGst::SeekTypeNone, QGst::ClockTime::None
         );

        pipeline->sendEvent(evt);
    }
}

void ArchiveWindow::onBusMessage(const QGst::MessagePtr& message)
{
    qDebug() << message->typeName() << " " << message->source()->property("name").toString();

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
        qDebug() << "EOS";
        pipeline->setState(QGst::StateReady);
        break;
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
        {
            QGst::SeekingQueryPtr queryLen = QGst::SeekingQuery::create(QGst::FormatTime);
            pipeline->query(queryLen);
            qDebug() << " seekable " << queryLen->seekable() << " format " << queryLen->format();
            auto isClip = queryLen->seekable();
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
//    qDebug() << message->oldState() << " => " << message->newState();

    if (message->source() == pipeline && message->oldState() == QGst::StateReady &&  message->newState() == QGst::StatePaused)
    {
        {
            QGst::SeekingQueryPtr queryLen = QGst::SeekingQuery::create(QGst::FormatTime);
            pipeline->query(queryLen);
            qDebug() << "FIRST seekable " << queryLen->seekable() << " format " << queryLen->format();
            auto isClip = queryLen->seekable();
            Q_FOREACH(auto action, barMediaControls->actions())
            {
                action->setVisible(isClip);
            }
        }
        {
            QGst::SeekEventPtr evt = QGst::SeekEvent::create(
                 1.0, QGst::FormatTime, QGst::SeekFlagFlush,
                 QGst::SeekTypeCur, 0,
                 QGst::SeekTypeNone, QGst::ClockTime::None
             );

            pipeline->sendEvent(evt);
        }
    }
}
