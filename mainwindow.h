#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>

class MainWindow : public QWidget
{
    bool recordAll;
    bool running;
    bool recording;

    QPushButton* btnRecordAll;
    QPushButton* btnStart;
    QPushButton* btnRecord;

    QPushButton* createButton(const char *slot);

    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void onRecordAllClick();

};

#endif // MAINWINDOW_H
