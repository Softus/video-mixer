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

#ifndef ARCHIVEWINDOW_H
#define ARCHIVEWINDOW_H

#include <QDialog>
#include <QDir>
#include <QListView>
#include <QSettings>

#include <QGst/Message>
#include <QGst/Pipeline>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QToolBar;
QT_END_NAMESPACE

class ArchiveWindow : public QWidget
{
    Q_OBJECT
    QToolBar*              barPath;
    QToolBar*              barMediaControls;
    QAction*               actionDelete;
#ifdef WITH_DICOM
    QAction*               actionStore;
#endif
    QAction*               actionBack;
    QAction*               actionEdit;
    QAction*               actionPlay;
    QAction*               actionMode;
    QAction*               actionListMode;
    QAction*               actionIconMode;
    QAction*               actionGalleryMode;
    QAction*               actionRestore;
    QAction*               actionSeekBack;
    QAction*               actionSeekFwd;
    QAction*               actionBrowse;
    QAction*               actionEnter;
    QAction*               actionUp;
    QListWidget*           listFiles;
    QStackedWidget*        pagesWidget;
    QWidget*               player;
    QGst::PipelinePtr      pipeline;
    QDir                   root;
    QDir                   curr;
    QFileSystemWatcher*    dirWatcher;
    QStringList            deleteLater;
    int                    updateTimerId;

    void reallyDeleteFiles();
    void queueFileDeletion(QListWidgetItem* item);
    void stopMedia();
    void playMediaFile(const QFileInfo &fi);
    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void createSubDirMenu(QAction* parentAction);
    void switchViewMode(int mode);

public:
    explicit ArchiveWindow(QWidget *parent = 0);
    ~ArchiveWindow();
protected:
    virtual void showEvent(QShowEvent *);
    virtual void timerEvent(QTimerEvent *);

signals:
    
public slots:
    void updateRoot();
    void updateHotkeys(QSettings &settings);
    void updatePath();
    void updateList();
    void setPath(const QString& path);
    void selectFile(const QString& fileName);
    void onListRowChanged(int idx);
    void onListKey();
    void onSwitchModeClick();
    void onShowFolderClick();
    void onDeleteClick();
    void onEditClick();
#ifdef WITH_DICOM
    void onStoreClick();
#endif
    void onBackToMainWindowClick();
    void onPrevClick();
    void onNextClick();
    void onSeekClick();
    void onPlayPauseClick();
    void preparePathPopupMenu();

private slots:
    void selectPath();
    void selectPath(QAction* action);
    void onSwitchModeClick(QAction* action);
    void onListItemDoubleClicked(QListWidgetItem* item);
    void onListItemDraggedOut(QListWidgetItem* item);
    void onDirectoryChanged(const QString&);
    void onUpFolderClick();
    void onRestoreClick();
};

#endif // ARCHIVEWINDOW_H
