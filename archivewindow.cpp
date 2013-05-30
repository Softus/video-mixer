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
#include <QStackedWidget>
#include <QToolBar>
#include <QUrl>

#include <QGlib/Connect>
#include <QGlib/Error>
#include <QGst/Bus>
#include <QGst/Parse>
#include <gst/gstdebugutils.h>

ArchiveWindow::ArchiveWindow(QWidget *parent) :
    QDialog(parent)
{
    setMinimumSize(QSize(1366, 768));
    QSettings settings;
    settings.beginGroup("archive");
    auto previewVisible = settings.value("preview-pane", true).toBool();

    QBoxLayout* layoutMain = new QVBoxLayout;

    QToolBar* barArchive = new QToolBar(tr("archive"));
    barArchive->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    actionDelete = barArchive->addAction(QIcon(":buttons/delete"), tr("Delete"), this, SLOT(onDeleteClick()));
    actionDelete->setEnabled(false);
    barArchive->addAction(QIcon(":buttons/folder"), tr("File browser"), this, SLOT(onShowFolderClick()));

    QWidget* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    barArchive->addWidget(spacer);

    auto actionListMode = barArchive->addAction(nullptr, this, SLOT(onToggleListModeClick()));
    auto actionShowPreview = barArchive->addAction(QIcon(":buttons/gallery"), tr("Show preview pane"), this, SLOT(onTogglePreviewClick()));
    actionShowPreview->setCheckable(true);
    actionShowPreview->setChecked(previewVisible);
    layoutMain->addWidget(barArchive);

    barPath = new QToolBar(tr("path"));
    layoutMain->addWidget(barPath);

    QBoxLayout* layoutContent = new QHBoxLayout;

    listFiles = new QListWidget;
    listFiles->setFocusPolicy(Qt::NoFocus);
    setListViewMode(actionListMode, (QListView::ViewMode)settings.value("list-mode", QListView::IconMode).toInt());
    listFiles->setMinimumHeight(160);
    listFiles->setMovement(QListView::Static);

    connect(listFiles, SIGNAL(currentTextChanged(QString)), this, SLOT(listItemSelected(QString)));
    layoutContent->addWidget(listFiles);

    previewPane = new QStackedWidget;
    previewPane->setVisible(previewVisible);
    previewPane->setMinimumSize(720, 576);
    previewPane->addWidget(lblImage = new QLabel);
    previewPane->addWidget(displayWidget = new QGst::Ui::VideoWidget());

    layoutContent->addWidget(previewPane);
    layoutMain->addLayout(layoutContent);

    setLayout(layoutMain);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void ArchiveWindow::setListViewMode(QAction* action, QListView::ViewMode mode)
{
    listFiles->setViewMode(mode);
    listFiles->setIconSize(mode == QListView::IconMode? QSize(144,144): QSize(32, 32));
    action->setIcon(QIcon(mode == QListView::IconMode? ":/buttons/list": ":/buttons/icons"));
    action->setText(mode == QListView::IconMode? tr("View as list"): tr("View as icon"));
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
        connect(menu, SIGNAL(aboutToShow()), this, SLOT(preparePopupMenu()));
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

void ArchiveWindow::preparePopupMenu()
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

    listFiles->clear();
    QFlags<QDir::Filter> filter = QDir::NoDot | QDir::AllEntries;
    if (curr == root)
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
                    continue;
                }
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

void ArchiveWindow::onToggleListModeClick()
{
    QSettings settings;
    settings.beginGroup("archive");

    auto action = static_cast<QAction*>(sender());
    auto viewMode = listFiles->viewMode() == QListView::ListMode? QListView::IconMode: QListView::ListMode;
    settings.setValue("list-mode", viewMode);
    setListViewMode(action, viewMode);
}

void ArchiveWindow::onTogglePreviewClick()
{
    QSettings settings;
    settings.beginGroup("archive");

    auto action = static_cast<QAction*>(sender());
    settings.setValue("preview-pane", action->isChecked());
    previewPane->setVisible(action->isChecked());
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
        removeFileOrFolder(curr.absoluteFilePath(item->text()));
        delete item;
    }
}

void ArchiveWindow::stopMedia()
{
    setWindowTitle(tr("Archive"));
    lblImage->clear();

    if (pipeline)
    {
        displayWidget->stopPipelineWatch();
        pipeline->setState(QGst::StateNull);
        pipeline->getState(nullptr, nullptr, 0);
        pipeline.clear();
    }
}

void ArchiveWindow::playMediaFile(const QString& file)
{
    stopMedia();

    QPixmap pm;
    if (pm.load(file))
    {
        lblImage->setPixmap(pm);
        previewPane->setCurrentWidget(lblImage);
        setWindowTitle(tr("Archive - %1").arg(file));
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
        pipeline->setState(QGst::StatePlaying);
        previewPane->setCurrentWidget(displayWidget);
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
    qDebug() << message->typeName() << " " << message->source()->property("name").toString();

    switch (message->type())
    {
    case QGst::MessageStateChanged:
//        onStateChangedMessage(message.staticCast<QGst::StateChangedMessage>());
        break;
    case QGst::MessageElement:
//        onElementMessage(message.staticCast<QGst::ElementMessage>());
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
    case QGst::MessageNewClock:
    case QGst::MessageStreamStatus:
    case QGst::MessageQos:
    case QGst::MessageAsyncDone:
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
