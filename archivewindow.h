#ifndef ARCHIVEWINDOW_H
#define ARCHIVEWINDOW_H

#include <QDialog>
#include <QDir>
#include <QListView>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

QT_BEGIN_NAMESPACE
class QToolBar;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
QT_END_NAMESPACE

class ArchiveWindow : public QDialog
{
    Q_OBJECT
    QToolBar*              barPath;
    QAction*               actionDelete;
    QListWidget*           listFiles;
    QStackedWidget*        previewPane;
    QLabel*                lblImage;
    QGst::Ui::VideoWidget* displayWidget;
    QGst::PipelinePtr      pipeline;
    QDir                   root;
    QDir                   curr;

    void updatePath();
    void updateList();
    void stopMedia();
    void playMediaFile(const QString& file);
    void onBusMessage(const QGst::MessagePtr& message);
    void createSubDirMenu(QAction* parentAction);
    void setListViewMode(QAction* action, QListView::ViewMode mode);

public:
    explicit ArchiveWindow(QWidget *parent = 0);
    
signals:
    
public slots:
    void setRoot(const QString& root);
    void setPath(const QString& path);
    void selectPath(QAction* action);
    void selectPath(bool);
    void listItemSelected(QString item);
    void onToggleListModeClick();
    void onTogglePreviewClick();
    void onShowFolderClick();
    void onDeleteClick();
    void preparePopupMenu();
};

#endif // ARCHIVEWINDOW_H
