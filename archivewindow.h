#ifndef ARCHIVEWINDOW_H
#define ARCHIVEWINDOW_H

#include <QDialog>
#include <QDir>
#include <QListView>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QToolBar;
QT_END_NAMESPACE

class ArchiveWindow : public QDialog
{
    Q_OBJECT
    QToolBar*              barPath;
    QToolBar*              barMediaControls;
    QAction*               actionDelete;
    QAction*               actionPlay;
    QAction*               actionMode;
    QListWidget*           listFiles;
    QGst::Ui::VideoWidget* displayWidget;
    QWidget*               player;
    QGst::PipelinePtr      pipeline;
    QDir                   root;
    QDir                   curr;

    void updatePath();
    void updateList();
    void stopMedia();
    void playMediaFile(const QString& file);
    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void createSubDirMenu(QAction* parentAction);
    void switchViewMode(int mode);

public:
    explicit ArchiveWindow(QWidget *parent = 0);
    ~ArchiveWindow();
protected:
    virtual void closeEvent(QCloseEvent *);

signals:
    
public slots:
    void setRoot(const QString& root);
    void setPath(const QString& path);
    void selectPath(QAction* action);
    void selectPath(bool);
    void onListItemDoubleClicked(QListWidgetItem* item);
    void onListRowChanged(int idx);
    void onListKey();
    void onSwitchModeClick();
    void onSwitchModeClick(QAction* action);
    void onShowFolderClick();
    void onDeleteClick();
    void onPrevClick();
    void onNextClick();
    void onSeekClick();
    void onPlayPauseClick();
    void preparePathPopupMenu();
};

#endif // ARCHIVEWINDOW_H
