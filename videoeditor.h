/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
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

#ifndef VIDEOEDITOR_H
#define VIDEOEDITOR_H

#include <QWidget>

#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
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
    void onCutClick();
    void onSaveAsClick();
    void onSaveClick();
    void onSnapshotClick();
    void onSeekClick();
    void setLowerPosition(int position);
    void setUpperPosition(int position);

private:
    QString                filePath;
    QxtSpanSlider*         sliderRange;
    QLabel*                lblStart;
    QLabel*                lblStop;
    QAction*               actionPlay;
    QAction*               actionSeekBack;
    QAction*               actionSeekFwd;
    QGst::Ui::VideoWidget* videoWidget;
    QGst::PipelinePtr      pipeline;
    qint64                 duration;

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChange(const QGst::StateChangedMessagePtr& message);
    bool exportVideo(QFile* outFile);
    void setPosition(int position, QLabel* lbl);
};

#endif // VIDEOEDITOR_H
