#ifndef VIDEOEDITOR_H
#define VIDEOEDITOR_H

#include <QWidget>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
class QSlider;
class QTimer;
class QxtSpanSlider;
QT_END_NAMESPACE

class VideoEditor : public QWidget
{
    Q_OBJECT
public:
    explicit VideoEditor(QWidget *parent = 0);
    ~VideoEditor();
    void loadFile(const QString& filePath);

protected:
    virtual void closeEvent(QCloseEvent *);

signals:

public slots:
    void onPlayPauseClick();
    void onPositionChanged();
    void onSaveAsClick();
    void onSaveClick();
    void onSeekClick();
    void setPosition(int position);

private:
    QString                filePath;
    QSlider*               sliderPos;
    QxtSpanSlider*         sliderRange;
    QLabel*                lblPos;
    QLabel*                lblRange;
    QGst::Ui::VideoWidget* videoWidget;
    QGst::PipelinePtr      pipeline;
    QTimer*                positionTimer;
    qint64                 duration;

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChange(const QGst::StateChangedMessagePtr& message);
    void exportVideo(QFile* outFile);
};

#endif // VIDEOEDITOR_H
