#ifndef ARCHIVEWINDOW_H
#define ARCHIVEWINDOW_H

#include <QDialog>
#include <QDir>
#include <QModelIndex>

QT_BEGIN_NAMESPACE
class QToolBar;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

class ArchiveWindow : public QDialog
{
    Q_OBJECT
    QToolBar* barPath;
    QListWidget* listFiles;
    QDir      root;
    QDir      curr;
    void updatePathBar();
    void updateList();

public:
    explicit ArchiveWindow(QWidget *parent = 0);
    
signals:
    
public slots:
    void setRoot(const QString& root);
    void setPath(const QString& path);
    void selectPath(QAction* action);
    void listItemSelected(QString item);
    void listItemDblClicked(QModelIndex idx);
    void onShowGalleryClick();
    void onShowFolderClick();
};

#endif // ARCHIVEWINDOW_H
