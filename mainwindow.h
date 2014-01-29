/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
#include <QGst/Message>
#include <QGst/Pipeline>
#include <QGst/Element>
#include <QGst/Pad>
#include <QGst/Buffer>
#include <QGst/Ui/VideoWidget>

#include <QGlib/Error>
#include <QGlib/Value>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMenuBar;
class QResizeEvent;
class QTimer;
class QToolBar;
class QToolButton;
QT_END_NAMESPACE

class Worklist;
class ArchiveWindow;
class DcmDataset;
class SlidingStackedWidget;
typedef struct _cairo cairo_t;

class MainWindow : public QWidget
{
    Q_OBJECT

    // UI
    //
    QToolButton* btnStart;
    QToolButton* btnRecord;
    QToolButton* btnSnapshot;
    QAction*     actionAbout;
    QAction*     actionSettings;
    QAction*     actionArchive;
    ArchiveWindow* archiveWindow;
#ifdef WITH_DICOM
    QAction*      actionWorklist;
    DcmDataset*   pendingPatient;
    Worklist*     worklist;
    QString       pendingSOPInstanceUID;
#endif
#ifdef WITH_TOUCH
    SlidingStackedWidget* mainStack;
#endif
    QGst::Ui::VideoWidget* displayWidget;
    QListWidget*  listImagesAndClips;
    QDir          outputPath;
    QString       clipPreviewFileName;
    QString       pipelineDef;

    QString       patientId;
    QString       patientSex;
    QString       patientName;
    QString       patientBirthDate;
    QString       physician;
    QString       studyName;

    int           imageNo;
    int           clipNo;
    int           studyNo;

    QMenuBar* createMenuBar();
    QToolBar* createToolBar();
    void updateStartButton();
    void updateRecordButton();

    // State machine
    //
    bool running;
    bool recording;

    // GStreamer pipeline
    //
    QGst::PipelinePtr pipeline;
    QGst::ElementPtr displaySink;
    QGst::ElementPtr imageValve;
    QGst::ElementPtr imageSink;
    QGst::ElementPtr videoEncoder;
    QGst::ElementPtr videoEncoderValve;

    QString replace(QString str, int seqNo = 0);
    QString buildPipeline();
    QGst::PipelinePtr createPipeline();
    void releasePipeline();
    void updateWindowTitle();
    void updateOutputPath();
    QString appendVideoTail(const QString& prefix, int idx);
    void removeVideoTail(const QString& prefix);

    void onBusMessage(const QGst::MessagePtr& message);
    void onStateChangedMessage(const QGst::StateChangedMessagePtr& message);
    void onElementMessage(const QGst::ElementMessagePtr& message);

    void onImageReady(const QGst::BufferPtr&);
    void onClipFrame(const QGst::BufferPtr&);
    void onVideoFrame(const QGst::BufferPtr&);
    void errorGlib(const QGlib::ObjectPtr& obj, const QGlib::Error& ex);
    void setElementProperty(const char* elm, const char* prop = nullptr, const QGlib::Value& value = nullptr, QGst::State minimumState = QGst::StatePlaying);
    void setElementProperty(QGst::ElementPtr& elm, const char* prop = nullptr, const QGlib::Value& value = nullptr, QGst::State minimumState = QGst::StatePlaying);
    bool startVideoRecord();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    virtual void closeEvent(QCloseEvent *);
signals:
    void enableWidget(QWidget*, bool);

private slots:
#ifdef WITH_DICOM
    void onShowWorkListClick();
    void onStartStudy(DcmDataset* patient = nullptr);
#else
    void onStartStudy();
#endif
    void onShowAboutClick();
    void onShowArchiveClick();
    void onShowSettingsClick();
    void onStartClick();
    void onSnapshotClick();
    void onRecordClick();
    void prepareSettingsMenu();
    void toggleSetting();
    void updatePipeline();
    void onEnableWidget(QWidget*, bool);
    void onThumbnailItemDoubleClicked(QListWidgetItem* item);
};

#endif // MAINWINDOW_H
