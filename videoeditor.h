#ifndef VIDEOEDITOR_H
#define VIDEOEDITOR_H

#include <QWidget>

class VideoEditor : public QWidget
{
    Q_OBJECT
public:
    explicit VideoEditor(QWidget *parent = 0);
    void loadFile(const QString& filePath);

protected:
    virtual void closeEvent(QCloseEvent *);

signals:

public slots:

};

#endif // VIDEOEDITOR_H
