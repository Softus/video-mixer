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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDir>
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

class ArchiveWindow;
class DcmDataset;
class Pipeline;
class SlidingStackedWidget;
class Sound;
class Worklist;
class PatientDataDialog;

class MainWindow : public QWidget
{
    Q_OBJECT

    // UI
    //
    PatientDataDialog *dlgPatient;
    QLabel*      extraTitle;
    QToolButton* btnStart;
    QToolButton* btnRecordStart;
    QToolButton* btnRecordStop;
    QToolButton* btnSnapshot;
    QAction*     actionAbout;
    QAction*     actionSettings;
    QAction*     actionArchive;
    ArchiveWindow* archiveWindow;
    QBoxLayout*  layoutSources;
    QBoxLayout*  layoutVideo;
#ifdef WITH_DICOM
    QAction*      actionWorklist;
    DcmDataset*   pendingPatient;
    Worklist*     worklist;
    QString       pendingSOPInstanceUID;
#endif
    SlidingStackedWidget* mainStack;
    QListWidget*  listImagesAndClips;
    QDir          outputPath;
    QDir          videoOutputPath;
    QString       clipPreviewFileName;

    QString       accessionNumber;
    QString       patientId;
    QString       patientSex;
    QString       patientName;
    QString       patientBirthDate;
    QString       physician;
    QString       studyName;

    ushort        imageNo;
    ushort        clipNo;
    int           studyNo;
    int           recordTimerId;
    int           recordLimit;
    int           recordNotify;
    int           countdown;
    Sound*        sound;

    QMenuBar* createMenuBar();
    QToolBar* createToolBar();
    void updateStartButton();

    // State machine
    //
    bool running;

    // GStreamer pipelines
    //
    Pipeline*        activePipeline;
    QList<Pipeline*> pipelines;
    QSize            mainSrcSize;
    QSize            altSrcSize;

    QString replace(QString str, int seqNo = 0);
    void updateWindowTitle();
    QDir checkPath(const QString tpl, bool needUnique);
    void updateOutputPath(bool needUnique);
    bool checkPipelines();
    void rebuildPipelines();
    void createPipeline(int index, int order);

    bool startVideoRecord();
    void updateStartDialog();
    bool confirmStopStudy();
    bool takeSnapshot(Pipeline* pipeline = nullptr, const QString& imageTemplate = QString());
    bool startRecord(int duration = 0, const QString &clipFileTemplate = QString());

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    virtual void closeEvent(QCloseEvent*);
    virtual void showEvent(QShowEvent*);
    virtual void hideEvent(QHideEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void timerEvent(QTimerEvent*);
signals:
    void enableWidget(QWidget*, bool);
    void updateOverlayText(int);

public slots:
    void applySettings();
    void toggleSetting();

private slots:
#ifdef WITH_DICOM
    void onShowWorkListClick();
    void onStartStudy(DcmDataset* patient = nullptr);
#else
    void onStartStudy();
#endif
    void onClipFrameReady();
    void onEnableWidget(QWidget*, bool);
    void onImageSaved(const QString& filename, const QString& tooltip, const QPixmap& pm);
    void onMotion(bool detected);
    void onPipelineError(const QString& text);
    void onPrepareSettingsMenu();
    void onRecordStartClick();
    void onRecordStopClick();
    void onShowAboutClick();
    void onShowArchiveClick();
    void onShowSettingsClick();
    void onSnapshotClick();
    void onSourceClick();
    void onSourceSnapshot();
    void onStartClick();
    void onStopStudy();
    void onSwapSources(QWidget*);
    void onVideoFrameReady();

    friend class MainWindowDBusAdaptor;
};

#endif // MAINWINDOW_H
