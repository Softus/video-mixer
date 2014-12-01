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

#include "mainwindow.h"
#include "product.h"
#include "aboutdialog.h"
#include "archivewindow.h"
#include "defaults.h"
#include "pipeline.h"
#include "patientdatadialog.h"
#include "qwaitcursor.h"
#include "settingsdialog.h"
#include "smartshortcut.h"
#include "sound.h"
#include "thumbnaillist.h"

#ifdef WITH_DICOM
#include "dicom/worklist.h"
#include "dicom/dcmclient.h"
#include "dicom/transcyrillic.h"

// From DCMTK SDK
//
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdict.h>
#include <dcmtk/dcmdata/dcdicent.h>
#include <dcmtk/dcmdata/dcelem.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdeftag.h>

static DcmTagKey DCM_ImageNo(0x5000, 0x8001);
static DcmTagKey DCM_ClipNo(0x5000,  0x8002);
#endif

#ifdef WITH_TOUCH
#include "touch/slidingstackedwidget.h"
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QxtConfirmationMessage>

#define SAFE_MODE_KEYS (Qt::AltModifier | Qt::ControlModifier | Qt::ShiftModifier)

#ifdef Q_OS_WIN
  #define DATA_FOLDER qApp->applicationDirPath()
  #ifndef FILE_ATTRIBUTE_HIDDEN
    #define FILE_ATTRIBUTE_HIDDEN 0x00000002
    extern "C" __declspec(dllimport) int __stdcall SetFileAttributesW(const wchar_t* lpFileName, quint32 dwFileAttributes);
  #endif
#else
  #define DATA_FOLDER qApp->applicationDirPath() + "/../share/" PRODUCT_SHORT_NAME
#endif

static inline QBoxLayout::Direction bestDirection(const QSize &s)
{
    return s.width() >= s.height()? QBoxLayout::LeftToRight: QBoxLayout::TopToBottom;
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    dlgPatient(nullptr),
    archiveWindow(nullptr),
#ifdef WITH_DICOM
    pendingPatient(nullptr),
    worklist(nullptr),
#endif
    imageNo(0),
    clipNo(0),
    studyNo(0),
    recordTimerId(0),
    recordLimit(0),
    recordNotify(0),
    countdown(0),
    running(false),
    activePipeline(nullptr)
{
    QSettings settings;
    studyNo = settings.value("study-no").toInt();

    // This magic required for updating widgets from worker threads on Microsoft (R) Windows (TM)
    //
    connect(this, SIGNAL(enableWidget(QWidget*, bool)), this, SLOT(onEnableWidget(QWidget*, bool)), Qt::QueuedConnection);

    auto layoutMain = new QVBoxLayout();
    extraTitle = new QLabel;
    auto font = extraTitle->font();
    font.setPointSize(font.pointSize() * 1.5);
    extraTitle->setFont(font);
    layoutMain->addWidget(extraTitle);
#ifndef WITH_TOUCH
    layoutMain->addWidget(createToolBar());
#endif
    listImagesAndClips = new ThumbnailList();
    listImagesAndClips->setViewMode(QListView::IconMode);
    listImagesAndClips->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    listImagesAndClips->setMinimumHeight(144); // 576/4
    listImagesAndClips->setMaximumHeight(176);
    listImagesAndClips->setIconSize(QSize(144,144));
    listImagesAndClips->setMovement(QListView::Snap);

#ifdef WITH_TOUCH
    mainStack = new SlidingStackedWidget();
    layoutMain->addWidget(mainStack);

    auto studyLayout = new QVBoxLayout;
    layoutVideo = new QHBoxLayout;
    layoutSources = new QVBoxLayout;
    layoutVideo->addLayout(layoutSources);
    studyLayout->addLayout(layoutVideo);
    studyLayout->addWidget(listImagesAndClips);
    studyLayout->addWidget(createToolBar());
    auto studyWidget = new QWidget;
    studyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    studyWidget->setLayout(studyLayout);
    studyWidget->setObjectName("Main");
    mainStack->addWidget(studyWidget);
    mainStack->setCurrentWidget(studyWidget);

    archiveWindow = new ArchiveWindow();
    archiveWindow->updateRoot();
    archiveWindow->setObjectName("Archive");
    mainStack->addWidget(archiveWindow);
#else
    layoutMain->addWidget(displayWidget);
    layoutMain->addWidget(listImagesAndClips);
#endif

    settings.beginGroup("ui");
    altSrcSize = settings.value("alt-src-size", DEFAULT_ALT_SRC_SIZE).toSize();
    mainSrcSize = settings.value("main-src-size", DEFAULT_MAIN_SRC_SIZE).toSize();
    extraTitle->setVisible(settings.value("extra-title").toBool());
    if (settings.value("enable-menu").toBool())
    {
        layoutMain->setMenuBar(createMenuBar());
    }
    setLayout(layoutMain);

    restoreGeometry(settings.value("mainwindow-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("mainwindow-state").toInt());
    settings.endGroup();

    settings.beginGroup("gst");
    auto nSources = settings.beginReadArray("src");

    if (nSources <= 0)
    {
        nSources = 1;
    }

    for (int i = 0; i < nSources; ++i)
    {
        auto p = new Pipeline(i, this);
        connect(p, SIGNAL(imageSaved(const QString&, const QString&, const QPixmap&)),
                this, SLOT(onImageSaved(const QString&, const QString&, const QPixmap&)), Qt::QueuedConnection);
        connect(p, SIGNAL(clipFrameReady()), this, SLOT(onClipFrameReady()), Qt::QueuedConnection);
        connect(p, SIGNAL(videoFrameReady()), this, SLOT(onVideoFrameReady()), Qt::QueuedConnection);
        connect(p, SIGNAL(pipelineError(const QString&)), this, SLOT(onPipelineError(const QString&)), Qt::QueuedConnection);
        connect(p, SIGNAL(motion(bool)), this, SLOT(onMotion(bool)), Qt::QueuedConnection);
        connect(this, SIGNAL(updateOverlayText(int)), p, SLOT(updateOverlayText(int)), Qt::QueuedConnection);
        connect(p->displayWidget, SIGNAL(swapWith(QWidget*)), this, SLOT(onSwapSources(QWidget*)));
        connect(p->displayWidget, SIGNAL(click()), this, SLOT(onSourceClick()));
        connect(p->displayWidget, SIGNAL(copy()), this, SLOT(onSourceSnapshot()));
        pipelines.push_back(p);

        settings.setArrayIndex(i);
        auto order = settings.value("order", -1).toInt();
        if (order < 0 && !activePipeline)
        {
            p->displayWidget->setMinimumSize(mainSrcSize);
            p->displayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            layoutVideo->insertWidget(0, p->displayWidget);
            activePipeline = p;
        }
        else
        {
            p->displayWidget->setMinimumSize(altSrcSize);
            p->displayWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            layoutSources->insertWidget(order < 0? nSources: order, p->displayWidget, 0, Qt::AlignTop);
        }
    }
    settings.endArray();

    layoutSources->addStretch();

    if (!activePipeline)
    {
        activePipeline = pipelines.front();
        activePipeline->displayWidget->setMinimumSize(mainSrcSize);
        activePipeline->displayWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layoutSources->removeWidget(activePipeline->displayWidget);
        layoutVideo->insertWidget(0, activePipeline->displayWidget);
    }

    settings.endGroup();

    updateStartButton();
    updateWindowTitle();

    sound = new Sound(this);

    outputPath.setPath(settings.value("storage/output-path", DEFAULT_OUTPUT_PATH).toString());

#ifdef WITH_DICOM
    DcmDataDictionary& d = dcmDataDict.wrlock();
    d.addEntry(new DcmDictEntry(DCM_ImageNo.getGroup(), DCM_ImageNo.getElement(), EVR_US, "ImageNo",
                                0, 0, nullptr, false, nullptr));
    d.addEntry(new DcmDictEntry(DCM_ClipNo.getGroup(), DCM_ClipNo.getElement(), EVR_US, "ClipNo",
                                0, 0, nullptr, false, nullptr));
    dcmDataDict.unlock();
#endif
}

MainWindow::~MainWindow()
{
    Q_FOREACH (auto p, pipelines)
    {
        delete p;
    }
    pipelines.clear();
    activePipeline = nullptr;

    delete archiveWindow;
    archiveWindow = nullptr;

#ifdef WITH_DICOM
    delete worklist;
    worklist = nullptr;
#endif
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    // Save keyboard modifiers state, since it may change after the confirm dialog will be shown.
    //
    auto fromMouse = qApp->keyboardModifiers() == Qt::NoModifier;

    // If a study is in progress, the user must confirm stoppping the app.
    //
    if (running && !confirmStopStudy())
    {
        evt->ignore();
        return;
    }

    // Don't trick the user, if she press the Alt+F4/Ctrl+Q.
    // Only for mouse clicks on close button.
    //
    if (fromMouse && QSettings().value("ui/hide-on-close").toBool())
    {
        evt->ignore();
        hide();

#ifndef WITH_TOUCH
        if (archiveWindow)
        {
            archiveWindow->hide();
        }
#ifdef WITH_DICOM
        if (worklist)
        {
            worklist->hide();
        }
#endif
#endif
        return;
    }

#ifndef WITH_TOUCH
    if (archiveWindow)
    {
        archiveWindow->close();
    }
#ifdef WITH_DICOM
    if (worklist)
    {
        worklist->close();
    }
#endif
#endif

    QWidget::closeEvent(evt);
}

void MainWindow::showEvent(QShowEvent *evt)
{
    if (!activePipeline->pipeline)
    {
        QSettings settings;
        auto safeMode    = settings.value("ui/enable-settings", DEFAULT_ENABLE_SETTINGS).toBool() && (
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
                       qApp->queryKeyboardModifiers() == SAFE_MODE_KEYS ||
#else
                       qApp->keyboardModifiers() == SAFE_MODE_KEYS ||
#endif
                       settings.value("safe-mode", true).toBool());

        if (safeMode)
        {
            settings.setValue("safe-mode", false);
            QTimer::singleShot(0, this, SLOT(onShowSettingsClick()));
        }
        else
        {
            updatePipeline();
        }
    }

    QWidget::showEvent(evt);
}

void MainWindow::hideEvent(QHideEvent *evt)
{
    QSettings settings;
    settings.beginGroup("ui");
    settings.setValue("mainwindow-geometry", saveGeometry());
    // Do not save window state, if it is in fullscreen mode or minimized
    //
    auto state = (int)windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen);
    settings.setValue("mainwindow-state", state);
    settings.endGroup();
    QWidget::hideEvent(evt);
}

void MainWindow::resizeEvent(QResizeEvent *evt)
{
    if (windowState().testFlag(Qt::WindowFullScreen))
    {
        extraTitle->setVisible(true);
    }
    QWidget::resizeEvent(evt);
}

void MainWindow::timerEvent(QTimerEvent* evt)
{
    if (evt->timerId() == recordTimerId)
    {
        if (--countdown <= recordNotify)
        {
            sound->play(DATA_FOLDER + "/sound/notify.ac3");
        }

        if (countdown == 0)
        {
            onRecordStopClick();
        }
        updateOverlayText(countdown);
    }
}

QMenuBar* MainWindow::createMenuBar()
{
    auto mnuBar = new QMenuBar();
    auto mnu    = new QMenu(tr("&Menu"));

    mnu->addAction(actionAbout);
    actionAbout->setMenuRole(QAction::AboutRole);

    mnu->addAction(actionArchive);

#ifdef WITH_DICOM
    mnu->addAction(actionWorklist);
#endif
    mnu->addSeparator();
    auto actionRtp = mnu->addAction(tr("&Enable RTP streaming"), this, SLOT(toggleSetting()));
    actionRtp->setCheckable(true);
    actionRtp->setData("gst/enable-rtp");

    auto actionFullVideo = mnu->addAction(tr("&Record entire study"), this, SLOT(toggleSetting()));
    actionFullVideo->setCheckable(true);
    actionFullVideo->setData("gst/enable-video");

    actionSettings->setMenuRole(QAction::PreferencesRole);
    mnu->addAction(actionSettings);
    mnu->addSeparator();
    auto actionExit = mnu->addAction(tr("E&xit"), qApp, SLOT(quit()), Qt::ALT | Qt::Key_F4);
    actionExit->setMenuRole(QAction::QuitRole);

    connect(mnu, SIGNAL(aboutToShow()), this, SLOT(onPrepareSettingsMenu()));
    mnuBar->addMenu(mnu);

    mnuBar->show();
    return mnuBar;
}

QToolBar* MainWindow::createToolBar()
{
    QToolBar* bar = new QToolBar(tr("Main"));

    btnStart = new QToolButton();
    btnStart->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnStart->setFocusPolicy(Qt::NoFocus);
    btnStart->setMinimumWidth(175);
    connect(btnStart, SIGNAL(clicked()), this, SLOT(onStartClick()));
    bar->addWidget(btnStart);

    btnSnapshot = new QToolButton();
    btnSnapshot->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSnapshot->setFocusPolicy(Qt::NoFocus);
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("Take snapshot"));
    btnSnapshot->setMinimumWidth(175);
    connect(btnSnapshot, SIGNAL(clicked()), this, SLOT(onSnapshotClick()));
    bar->addWidget(btnSnapshot);

    btnRecordStart = new QToolButton();
    btnRecordStart->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRecordStart->setFocusPolicy(Qt::NoFocus);
    btnRecordStart->setIcon(QIcon(":/buttons/record"));
    btnRecordStart->setText(tr("Start clip"));
    btnRecordStart->setMinimumWidth(175);
    connect(btnRecordStart, SIGNAL(clicked()), this, SLOT(onRecordStartClick()));
    bar->addWidget(btnRecordStart);

    btnRecordStop = new QToolButton();
    btnRecordStop->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRecordStop->setFocusPolicy(Qt::NoFocus);
    btnRecordStop->setIcon(QIcon(":/buttons/record_done"));
    btnRecordStop->setText(tr("Clip done"));
    btnRecordStop->setMinimumWidth(175);
    connect(btnRecordStop, SIGNAL(clicked()), this, SLOT(onRecordStopClick()));
    bar->addWidget(btnRecordStop);

    QWidget* spacer = new QWidget;
    spacer->setMinimumWidth(1);
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    bar->addWidget(spacer);

#ifdef WITH_DICOM
    actionWorklist = bar->addAction(QIcon(":/buttons/show_worklist"), tr("&Worlkist"),
                                    this, SLOT(onShowWorkListClick()));
#endif

    actionArchive = bar->addAction(QIcon(":/buttons/database"), tr("&Archive"), this, SLOT(onShowArchiveClick()));
    actionArchive->setToolTip(tr("Show studies archive"));

    actionSettings = bar->addAction(QIcon(":/buttons/settings"), tr("&Preferences").append(0x2026),
                                    this, SLOT(onShowSettingsClick()));
    actionSettings->setToolTip(tr("Edit settings"));

    actionAbout = bar->addAction(QIcon(":/buttons/about"), tr("A&bout %1").arg(PRODUCT_FULL_NAME).append(0x2026),
                                 this, SLOT(onShowAboutClick()));
    actionAbout->setToolTip(tr("About %1").arg(PRODUCT_FULL_NAME));

    return bar;
}

/*

  The pipeline is:

                 [video src]
                      |
                      V
          [video decoder, for DV/JPEG]
                      |
                      V
                [deinterlace]
                      |
                      V
         +----[main splitter]------+
         |            |            |
  [image valve]   [detector]  [video rate]
         |            |            |
         V            V            V
 [image encoder]  [display]   [video valve]
         |                         |
         V                         V
  [image writer]             [video encoder]
                                   |
                                   V
                       +----[video splitter]----+--------------+
                       |           |            |              |
                       V           V            V              V
            [movie writer]   [clip valve]  [rtp sender]  [http sender]
                                   |
                                   V
                            [clip writer]


Sample:
    v4l2src [! dvdemux ! ffdec_dvvideo | ! jpegdec] [! colorspace] [! deinterlace] ! tee name=splitter
        splitter. ! autovideosink name=displaysink async=0
        splitter. ! valve name=encvalve drop=1 ! queue max-size-bytes=0 ! videorate max-rate=30/1 ! x264enc name=videoencoder ! tee name=videosplitter
                videosplitter. ! identity  name=videoinspect drop-probability=1.0 ! queue ! valve name=videovalve drop=1 ! [mpegpsmux name=videomux ! filesink name=videosink]
                videosplitter. ! queue ! rtph264pay ! udpsink name=rtpsink clients=127.0.0.1:5000 sync=0
                videosplitter. ! identity  name=clipinspect drop-probability=1.0 ! queue ! valve name=clipvalve ! [ mpegpsmux name=clipmux ! filesink name=clipsink]
        splitter. ! identity name=imagevalve drop-probability=1.0 ! jpegenc ! multifilesink name=imagesink post-messages=1 async=0 sync=0 location=/video/image

                [video src]
                     |
                     V
         +----[video splitter]----+----------+
         |           |            |          |
         V           V            V          V
[movie writer] [clip valve] [rtp sender] [decoder]
                     |                       |
                     V                       V
               [clip writer]            [splitter]------+
                                             |          |
                                             V          V
                                      [image valve]  [detector]
                                             |          |
                                             V          V
                                     [image encoder] [display]
                                             |
                                             V
                                      [image writer]
*/

static QString appendVideo(QString& pipe, const QSettings& settings)
{
/*
                       +----[video splitter]----+-------------+
                       |           |            |             |
                       V           V            V             V
            [movie writer]   [clip valve]  [rtp sender]  [http sender]
                                   |
                                   V
                            [clip writer]

*/
    auto rtpPayDef       = settings.value("rtp-payloader",  DEFAULT_RTP_PAYLOADER).toString();
    auto rtpPayParams    = settings.value(rtpPayDef + "-parameters").toString();
    auto rtpSinkDef      = settings.value("rtp-sink",       DEFAULT_RTP_SINK).toString();
    auto rtpSinkParams   = settings.value(rtpSinkDef + "-parameters").toString();
    auto enableRtp       = !rtpSinkDef.isEmpty() && settings.value("enable-rtp").toBool();
    auto httpSinkDef     = settings.value("http-sink",      DEFAULT_HTTP_SINK).toString();
    auto enableHttp      = !httpSinkDef.isEmpty() && settings.value("enable-http").toBool();
    auto httpPushUrl     = settings.value("http-push-url").toString();
    auto httpSinkParams  = settings.value(httpSinkDef + "-parameters").toString();
    auto enableVideoLog  = settings.value("enable-video").toBool();

    pipe.append(" ! tee name=videosplitter");
    if (enableRtp || enableHttp || enableVideoLog)
    {
        if (enableVideoLog)
        {
            pipe.append("\nvideosplitter. ! identity name=videoinspect drop-probability=1.0 ! queue ! valve name=videovalve ");
        }

        if (enableRtp)
        {
            pipe.append("\nvideosplitter. ! queue ! ");

            // MPEG2TS is a payloader for container, so add the required muxer
            //
            if (rtpPayDef == "rtpmp2tpay")
            {
                pipe.append("mpegtsmux name=rtpmux ! ");
            }

            pipe.append(rtpPayDef).append(" ").append(rtpPayParams)
                .append(" ! ").append(rtpSinkDef).append(" clients=127.0.0.1:5000 sync=0 async=0 name=rtpsink ")
                .append(rtpSinkParams);
        }

        if (enableHttp && !httpPushUrl.isEmpty())
        {
            pipe.append("\nvideosplitter. ! queue ! mpegtsmux name=httpmux ! ").append(httpSinkDef)
                .append(" async=0 name=httpsink location=\"").append(httpPushUrl).append("\" ").append(httpSinkParams);
        }
    }

    return pipe.append("\nvideosplitter. ! identity name=clipinspect drop-probability=1.0 ! queue ! valve name=clipvalve drop=1");
}

QString MainWindow::buildPipeline()
{
    QSettings settings;

    // v4l2src device=/dev/video1 name=(channel) ! video/x-raw-yuv,format=YUY2,width=720,height=576 ! colorspace
    // dv1394src guid="9025895599807395" ! video/x-dv,format=PAL ! dvdemux ! dvdec ! colorspace
    //
    QString pipe;

    auto outputPathDef  = settings.value("storage/output-path",    DEFAULT_OUTPUT_PATH).toString();

    settings.beginGroup("gst");
    auto deviceType     = settings.value("device-type", PLATFORM_SPECIFIC_SOURCE).toString();
    auto deviceDef      = settings.value("device").toString();
    auto inputChannel   = settings.value("video-channel").toString();
    auto formatDef      = settings.value("format").toString();
    auto sizeDef        = settings.value("size").toSize();
    auto srcDeinterlace = settings.value("video-deinterlace").toBool();
    auto srcParams      = settings.value("src-parameters").toString();
    auto colorConverter = QString(" ! ").append(settings.value("color-converter", "ffmpegcolorspace").toString());
    auto videoCodec     = settings.value("video-encoder",  DEFAULT_VIDEO_ENCODER).toString();

    pipe.append(deviceType);

    if (deviceType == "dv1394src")
    {
        // Special handling of dv video sources
        //
        if (inputChannel.toInt() > 0)
        {
            pipe.append(" channel=").append(inputChannel).append("");
        }
        if (!deviceDef.isEmpty())
        {
            pipe.append(" guid=\"").append(deviceDef).append("\"");
        }
    }
    else
    {
        if (!inputChannel.isEmpty())
        {
            // Hack: since channel name can't be set with attributes,
            // we set element name instead. The one reason is to
            // make the pipeline text do not match with older one.
            //
            pipe.append(" name=\"").append(inputChannel).append("\"");
        }
        if (!deviceDef.isEmpty())
        {
            pipe.append(" " PLATFORM_SPECIFIC_PROPERTY "=\"").append(deviceDef).append("\"");
        }
    }

    if (!srcParams.isEmpty())
    {
        pipe.append(' ').append(srcParams);
    }

    if (!formatDef.isEmpty())
    {
        pipe.append(" ! ").append(formatDef);
        if (!sizeDef.isEmpty())
        {
            pipe = pipe.append(",width=%1,height=%2").arg(sizeDef.width()).arg(sizeDef.height());
        }
    }

    if (videoCodec.isEmpty())
    {
        appendVideo(pipe, settings);
        pipe.append("\nvideosplitter.");
    }

    auto formatType = formatDef.split(',').first();
    if (formatType == "image/jpeg")
    {
        pipe.append(" ! jpegdec");
    }
    else if (deviceType == "dv1394src" || formatType == "video/x-dv")
    {
        // Add dv demuxer & decoder for DV sources
        //
        pipe.append(" ! dvdemux ! dvdec");
    }

    pipe.append(colorConverter).append(srcDeinterlace? " ! deinterlace mode=1 method=4": "");

    // v4l2src ... ! tee name=splitter [! colorspace ! motioncells] ! colorspace ! autovideosink");
    //
    auto displaySinkDef  = settings.value("display-sink", DEFAULT_DISPLAY_SINK).toString();
    auto displayParams   = settings.value(displaySinkDef + "-parameters").toString();

    auto detectMotion    = settings.value("enable-video").toBool() &&
                           settings.value("detect-motion", DEFAULT_MOTION_DETECTION).toBool();
    pipe.append(" ! tee name=splitter");
    if (!displaySinkDef.isEmpty())
    {
        pipe.append("\nsplitter.").append(colorConverter);
        if (detectMotion)
        {
            auto motionDebug       = settings.value("motion-debug", false).toString();
            auto motionSensitivity = settings.value("motion-sensitivity", DEFAULT_MOTION_SENSITIVITY).toString();
            auto motionThreshold   = settings.value("motion-threshold",   DEFAULT_MOTION_THRESHOLD).toString();
            auto motionMinFrames   = settings.value("motion-min-frames",  DEFAULT_MOTION_MIN_FRAMES).toString();
            auto motionGap         = settings.value("motion-gap",         DEFAULT_MOTION_GAP).toString();

            pipe.append(" ! motioncells name=motion-detector display=").append(motionDebug)
                .append(" sensitivity=").append(motionSensitivity)
                .append(" threshold=").append(motionThreshold)
                .append(" minimummotionframes=").append(motionMinFrames)
                .append(" gap=").append(motionGap);
        }
        pipe.append(" ! textoverlay name=displayoverlay color=-1 halignment=right valignment=top text=* xpad=8 ypad=0 font-desc=16")
            .append(colorConverter)
            .append(" ! " ).append(displaySinkDef).append(" name=displaysink async=0 ").append(displayParams);
    }

    // ... splitter. ! identity name=imagevalve ! jpegenc ! multifilesink splitter.
    //
    auto imageEncoderDef = settings.value("image-encoder", DEFAULT_IMAGE_ENCODER).toString();
    auto imageEncoderFixColor = settings.value(imageEncoderDef + "-colorspace", false).toBool();
    auto imageEncoderParams = settings.value(imageEncoderDef + "-parameters").toString();
    auto imageSinkDef       = settings.value("image-sink", DEFAULT_IMAGE_SINK).toString();
    if (!imageSinkDef.isEmpty())
    {
        pipe.append("\nsplitter. ! identity name=imagevalve drop-probability=1.0")
            .append(imageEncoderFixColor? colorConverter: "")
            .append(" ! ").append(imageEncoderDef).append(" ").append(imageEncoderParams)
            .append(" ! ").append(imageSinkDef).append(" name=imagesink post-messages=1 async=0 sync=0 location=\"")
            .append(outputPathDef).append("/image\"");
    }

    if (!videoCodec.isEmpty())
    {
        // ... splitter. ! videorate ! valve name=encvalve ! colorspace ! x264enc
        //           ! tee name=videosplitter
        //                videosplitter. ! queue ! mpegpsmux ! filesink
        //                videosplitter. ! queue ! rtph264pay ! udpsink
        //                videosplitter. ! identity name=clipinspect ! queue ! mpegpsmux ! filesink
        //
        auto videoMaxRate       = settings.value("limit-video-fps", DEFAULT_LIMIT_VIDEO_FPS).toBool()?
                                  settings.value("video-max-fps",  DEFAULT_VIDEO_MAX_FPS).toInt(): 0;
        auto videoFixColor      = settings.value(videoCodec + "-colorspace").toBool();
        auto videoEncoderParams = settings.value(videoCodec + "-parameters").toString();
        auto noIdleStream       = settings.value("no-idle-stream").toBool();

        pipe.append("\nsplitter.");
        if (videoMaxRate > 0)
        {
            pipe.append(" ! videorate skip-to-first=1 max-rate=").append(QString::number(videoMaxRate));
        }

        if (noIdleStream)
        {
            pipe.append(" ! valve name=encvalve drop=1");
        }

        pipe.append(" ! queue max-size-bytes=0")
            .append(videoFixColor? colorConverter: "")
            .append(" ! ").append(videoCodec).append(" name=videoencoder ").append(videoEncoderParams);

        appendVideo(pipe, settings);
    }

    return pipe;
}

QGst::PipelinePtr MainWindow::createPipeline()
{
    qCritical() << pipelineDef;

    QGst::PipelinePtr pl;

    // Default values for all profiles
    //

    try
    {
        pl = QGst::Parse::launch(pipelineDef).dynamicCast<QGst::Pipeline>();
    }
    catch (const QGlib::Error& ex)
    {
        errorGlib(pl, ex);
    }

    if (pl)
    {
        QGlib::connect(pl->bus(), "message", this, &MainWindow::onBusMessage);
        pl->bus()->addSignalWatch();
        displayWidget->watchPipeline(pl);

        displaySink = pl->getElementByName("displaysink");
        if (!displaySink)
        {
            qCritical() << "Element displaysink not found";
        }

        auto clipInspect = pl->getElementByName("clipinspect");
        clipInspect && QGlib::connect(clipInspect, "handoff", this, &MainWindow::onClipFrame);

        auto videoInspect = pl->getElementByName("videoinspect");
        videoInspect && QGlib::connect(videoInspect, "handoff", this, &MainWindow::onVideoFrame);

        displayOverlay    = pl->getElementByName("displayoverlay");
        videoEncoder      = pl->getElementByName("videoencoder");

        imageValve = pl->getElementByName("imagevalve");
        imageValve && QGlib::connect(imageValve, "handoff", this, &MainWindow::onImageReady);

        imageSink  = pl->getElementByName("imagesink");
        if (!imageSink)
        {
            qCritical() << "Element imagesink not found";
        }

        auto flags = GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES;
        auto details = GstDebugGraphDetails(flags);
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(pl.staticCast<QGst::Bin>(), details, qApp->applicationName().toUtf8());

        // The pipeline will start once it reaches paused state without an error
        //
        pl->setState(QGst::StatePlaying);
    }

    return pl;
}

void MainWindow::setElementProperty(const char* elmName, const char* prop, const QGlib::Value& value, QGst::State minimumState)
{
    auto elm = pipeline? pipeline->getElementByName(elmName): QGst::ElementPtr();
    if (!elm)
    {
        qDebug() << "Element " << elmName << " not found";
    }
    else
    {
        setElementProperty(elm, prop, value, minimumState);
    }
}

void MainWindow::setElementProperty(QGst::ElementPtr& elm, const char* prop, const QGlib::Value& value, QGst::State minimumState)
{
    if (elm)
    {
        QGst::State currentState = QGst::StateVoidPending;
        elm->getState(&currentState, nullptr, 1000000000L); // 1 sec
        if (currentState > minimumState)
        {
            elm->setState(minimumState);
            elm->getState(nullptr, nullptr, 1000000000L);
        }
        if (prop)
        {
            //qDebug() << elm->name() << prop << value.toString();
            elm->setProperty(prop, value);
        }
        elm->setState(currentState);
        elm->getState(nullptr, nullptr, 1000000000L);
    }
}

// mpegpsmux => mpg, jpegenc => jpg, pngenc => png, oggmux => ogg, avimux => avi, matrosskamux => mat
//
static QString getExt(QString str)
{
    if (str.startsWith("ffmux_"))
    {
        str = str.mid(6);
    }
    return QString(".").append(str.remove('e').left(3));
}

static QString fixFileName(QString str)
{
    if (!str.isNull())
    {
        for (int i = 0; i < str.length(); ++i)
        {
            switch (str[i].unicode())
            {
            case '<':
            case '>':
            case ':':
            case '\"':
            case '/':
            case '\\':
            case '|':
            case '?':
            case '*':
                str[i] = '_';
                break;
            }
        }
    }
    return str;
}

QString MainWindow::replace(QString str, int seqNo)
{
    auto nn = seqNo >= 10? QString::number(seqNo): QString("0").append('0' + seqNo);
    auto ts = QDateTime::currentDateTime();

    return str
        .replace("%an%",        accessionNumber,     Qt::CaseInsensitive)
        .replace("%name%",      patientName,         Qt::CaseInsensitive)
        .replace("%id%",        patientId,           Qt::CaseInsensitive)
        .replace("%sex%",       patientSex,          Qt::CaseInsensitive)
        .replace("%birthdate%", patientBirthDate,    Qt::CaseInsensitive)
        .replace("%physician%", physician,           Qt::CaseInsensitive)
        .replace("%study%",     studyName,           Qt::CaseInsensitive)
        .replace("%yyyy%",      ts.toString("yyyy"), Qt::CaseInsensitive)
        .replace("%yy%",        ts.toString("yy"),   Qt::CaseInsensitive)
        .replace("%mm%",        ts.toString("MM"),   Qt::CaseInsensitive)
        .replace("%mmm%",       ts.toString("MMM"),  Qt::CaseInsensitive)
        .replace("%mmmm%",      ts.toString("MMMM"), Qt::CaseInsensitive)
        .replace("%dd%",        ts.toString("dd"),   Qt::CaseInsensitive)
        .replace("%ddd%",       ts.toString("ddd"),  Qt::CaseInsensitive)
        .replace("%dddd%",      ts.toString("dddd"), Qt::CaseInsensitive)
        .replace("%hh%",        ts.toString("hh"),   Qt::CaseInsensitive)
        .replace("%min%",       ts.toString("mm"),   Qt::CaseInsensitive)
        .replace("%ss%",        ts.toString("ss"),   Qt::CaseInsensitive)
        .replace("%zzz%",       ts.toString("zzz"),  Qt::CaseInsensitive)
        .replace("%ap%",        ts.toString("ap"),   Qt::CaseInsensitive)
        .replace("%nn%",        nn,                  Qt::CaseInsensitive)
        ;
}

void MainWindow::updatePipeline()
{
    QWaitCursor wait(this);

    Q_FOREACH (auto p, pipelines)
    {
        p->updatePipeline();
    }

    btnStart->setEnabled(activePipeline->pipeline);

    QSettings settings;
    settings.beginGroup("gst");

    recordNotify = settings.value("notify-clip-limit", DEFAULT_NOTIFY_CLIP_LIMIT).toBool()?
        settings.value("notify-clip-countdown", DEFAULT_NOTIFY_CLIP_COUNTDOWN).toInt(): -1;

    settings.endGroup();

    if (archiveWindow != nullptr)
    {
        archiveWindow->updateRoot();
        archiveWindow->updateHotkeys(settings);
    }

    auto showSettings = settings.value("ui/enable-settings", DEFAULT_ENABLE_SETTINGS).toBool();
    actionSettings->setEnabled(showSettings);
    actionSettings->setVisible(showSettings);

    settings.beginGroup("hotkeys");
    updateShortcut(btnStart,    settings.value("capture-start",    DEFAULT_HOTKEY_START).toInt());
    updateShortcut(btnSnapshot, settings.value("capture-snapshot", DEFAULT_HOTKEY_SNAPSHOT).toInt());
    updateShortcut(btnRecordStart, settings.value("capture-record-start", DEFAULT_HOTKEY_RECORD_START).toInt());
    updateShortcut(btnRecordStop,  settings.value("capture-record-stop",  DEFAULT_HOTKEY_RECORD_STOP).toInt());

    updateShortcut(actionArchive,  settings.value("capture-archive",   DEFAULT_HOTKEY_ARCHIVE).toInt());
    updateShortcut(actionSettings, settings.value("capture-settings",  DEFAULT_HOTKEY_SETTINGS).toInt());
    updateShortcut(actionAbout,    settings.value("capture-about",     DEFAULT_HOTKEY_ABOUT).toInt());

#ifdef WITH_DICOM
    // Recreate worklist just in case the columns/servers were changed
    //
    delete worklist;
    worklist = new Worklist();
    connect(worklist, SIGNAL(startStudy(DcmDataset*)), this, SLOT(onStartStudy(DcmDataset*)));
#ifdef WITH_TOUCH
    worklist->setObjectName("Worklist");
    mainStack->addWidget(worklist);
#endif
    updateShortcut(actionWorklist, settings.value("capture-worklist",   DEFAULT_HOTKEY_WORKLIST).toInt());
#endif
    settings.endGroup();

#ifdef WITH_DICOM
    actionWorklist->setEnabled(!settings.value("dicom/mwl-server").toString().isEmpty());
#endif

    updateOverlayText(countdown);
}

void MainWindow::updateWindowTitle()
{
    QString windowTitle(PRODUCT_FULL_NAME);
    if (running)
    {
//      if (!accessionNumber.isEmpty())
//      {
//          windowTitle.append(tr(" - ")).append(accessionNumber);
//      }

        if (!patientId.isEmpty())
        {
            windowTitle.append(tr(" - ")).append(patientId);
        }

        if (!patientName.isEmpty())
        {
            windowTitle.append(tr(" - ")).append(patientName);
        }

        if (!physician.isEmpty())
        {
            windowTitle.append(tr(" - ")).append(physician);
        }

        if (!studyName.isEmpty())
        {
            windowTitle.append(tr(" - ")).append(studyName);
        }
    }

    setWindowTitle(windowTitle);
    extraTitle->setText(windowTitle);
}

QDir MainWindow::checkPath(const QString tpl, bool needUnique)
{
    QDir dir(tpl);

    if (needUnique && dir.exists() && !dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
    {
        int cnt = 1;
        QString alt;
        do
        {
            alt = dir.absolutePath()
                .append(" (").append(QString::number(++cnt)).append(')');
        }
        while (dir.exists(alt) && !dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
        dir.setPath(alt);
    }
    qDebug() << "Output path" << dir.absolutePath();

    if (!dir.mkpath("."))
    {
        QString msg = tr("Failed to create '%1'").arg(dir.absolutePath());
        qCritical() << msg;
        QMessageBox::critical(this, windowTitle(), msg, QMessageBox::Ok);
    }

    return dir;
}

void MainWindow::updateOutputPath(bool needUnique)
{
    QSettings settings;
    settings.beginGroup("storage");

    auto root = settings.value("output-path", DEFAULT_OUTPUT_PATH).toString();
    auto tpl = settings.value("folder-template", DEFAULT_FOLDER_TEMPLATE).toString();
    auto path = replace(root + tpl, studyNo);
    outputPath = checkPath(path, needUnique && settings.value("output-unique", DEFAULT_OUTPUT_UNIQUE).toBool());

    auto videoRoot = settings.value("video-output-path").toString();
    if (videoRoot.isEmpty())
    {
        videoRoot = root;
    }
    auto videoTpl = settings.value("video-folder-template").toString();
    if (videoTpl.isEmpty())
    {
        videoTpl = tpl;
    }

    auto videoPath = replace(videoRoot + videoTpl, studyNo);

    // If video path is same as images path, omit checkPath,
    // since it is already checked.
    //
    videoOutputPath = (videoPath == path)? outputPath:
        checkPath(videoPath, needUnique && settings.value("video-output-unique", DEFAULT_VIDEO_OUTPUT_UNIQUE).toBool());
}

void MainWindow::onClipFrameReady()
{
    if (recordLimit > 0 && recordTimerId == 0)
    {
        countdown = recordLimit;
        recordTimerId = startTimer(1000);
    }
    enableWidget(btnRecordStart, true);
    enableWidget(btnRecordStop, true);
    updateOverlayText(countdown);

    if (!clipPreviewFileName.isEmpty())
    {
        auto pipeline = static_cast<Pipeline*>(sender());

        // Turn the valve on for a while.
        //
        pipeline->imageValve->setProperty("drop-probability", 0.0);

        // Once an image will be ready, the valve will be turned off again.
        //
        enableWidget(btnSnapshot, false);
    }
}

void MainWindow::onVideoFrameReady()
{
    updateOverlayText(countdown);
}

void MainWindow::onPipelineError(const QString& text)
{
    QMessageBox::critical(this, windowTitle(), text, QMessageBox::Ok);
    onStopStudy();
}

void MainWindow::onMotion(bool)
{
    updateOverlayText(countdown);
}

void MainWindow::onImageSaved(const QString& filename, const QString &tooltip, const QPixmap& pixmap)
{
    QPixmap pm = pixmap.copy();

    if (clipPreviewFileName == filename)
    {
        // Got a snapshot for a clip file. Add a fency overlay to it
        //
        QPixmap pmOverlay(":/buttons/film");
        QPainter painter(&pm);
        painter.setOpacity(0.75);
        painter.drawPixmap(pm.rect(), pmOverlay);
        clipPreviewFileName.clear();
#ifdef Q_OS_WIN
        SetFileAttributesW(fileName.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
    }

    auto baseName = QFileInfo(filename).completeBaseName();
    if (baseName.startsWith('.'))
    {
        baseName = QFileInfo(baseName.mid(1)).completeBaseName();
    }
    auto existent = listImagesAndClips->findItems(baseName, Qt::MatchExactly);
    auto item = !existent.isEmpty()? existent.first(): new QListWidgetItem(baseName, listImagesAndClips);
    item->setToolTip(tooltip);
    item->setIcon(QIcon(pm));
    item->setSizeHint(QSize(176, 144));
    listImagesAndClips->setItemSelected(item, true);
    listImagesAndClips->scrollToItem(item);

    btnSnapshot->setEnabled(running);
}

bool MainWindow::startVideoRecord()
{
    if (!activePipeline->pipeline)
    {
        // How we get here?
        //
        QMessageBox::critical(this, windowTitle(),
            tr("Failed to start recording.\nPlease, adjust the video source settings."), QMessageBox::Ok);
        return false;
    }

    QSettings settings;
    auto ok = true;
    if (settings.value("gst/enable-video").toBool())
    {
        auto split = settings.value("gst/split-video-files", DEFAULT_SPLIT_VIDEO_FILES).toBool();
        auto fileTemplate = settings.value("storage/video-template", DEFAULT_VIDEO_TEMPLATE).toString();

        Q_FOREACH (auto p, pipelines)
        {
            auto videoFileName = p->appendVideoTail(videoOutputPath, "video",
                 replace(fileTemplate, studyNo), split);

            if (videoFileName.isEmpty())
            {
                QMessageBox::critical(this, windowTitle(),
                    tr("Failed to start recording.\nCheck the error log for details."), QMessageBox::Ok);
                ok = false;
                break;
            }
        }

        if (ok)
        {
            Q_FOREACH (auto p, pipelines)
            {
                p->enableVideo(true);
            }
        }
        else
        {
            Q_FOREACH (auto p, pipelines)
            {
                p->removeVideoTail("video");
            }
        }
    }

    return ok;
}

void MainWindow::onStartClick()
{
    if (!running)
    {
        onStartStudy();
    }
    else
    {
        confirmStopStudy();
    }
}

bool MainWindow::confirmStopStudy()
{
    QxtConfirmationMessage msg(QMessageBox::Question, windowTitle()
        , tr("End the study?"), QString(), QMessageBox::Yes | QMessageBox::No, this);
    msg.setSettingsPath("confirmations");
    msg.setOverrideSettingsKey("end-study");
    msg.setRememberOnReject(false);
    msg.setDefaultButton(QMessageBox::Yes);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    if (qApp->queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
#else
    if (qApp->keyboardModifiers().testFlag(Qt::ShiftModifier))
#endif
    {
        msg.reset();
    }

    if (QMessageBox::Yes == msg.exec())
    {
        onStopStudy();

        // Open the archive window after end of study
        //
        QTimer::singleShot(100, this, SLOT(onShowArchiveClick()));
        return true;
    }

    return false;
}

void MainWindow::onSnapshotClick()
{
    takeSnapshot();
}

void MainWindow::onSourceSnapshot()
{
    Q_FOREACH (auto p, pipelines)
    {
        if (p->displayWidget == sender())
        {
            takeSnapshot(p);
            break;
        }
    }
}

void MainWindow::onSourceClick()
{
    if (activePipeline->displayWidget == sender())
    {
        takeSnapshot(activePipeline);
    }
    else
    {
        onSwapSources(activePipeline->displayWidget);
    }
}

bool MainWindow::takeSnapshot(Pipeline* pipeline, const QString& imageTemplate)
{
    if (!running)
    {
        return false;
    }

    if (!pipeline)
    {
        pipeline = activePipeline;
    }

    QSettings settings;
    auto imageExt = getExt(settings.value("gst/image-encoder", DEFAULT_IMAGE_ENCODER).toString());
    auto actualImageTemplate = !imageTemplate.isEmpty()? imageTemplate:
            settings.value("storage/image-template", DEFAULT_IMAGE_TEMPLATE).toString();
    auto imageFileName = replace(actualImageTemplate, ++imageNo).append(imageExt);

    sound->play(DATA_FOLDER + "/sound/shutter.ac3");

    pipeline->setImageLocation(outputPath.absoluteFilePath(imageFileName));

    // Turn the valve on for a while.
    //
    pipeline->imageValve->setProperty("drop-probability", 0.0);
    //
    // Once the image will be ready, the valve will be turned off again.
    btnSnapshot->setEnabled(false);
    return true;
}

void MainWindow::onRecordStartClick()
{
    startRecord(0);
}

bool MainWindow::startRecord(int duration, const QString &clipFileTemplate)
{
    if (!running)
    {
        return false;
    }

    QSettings settings;
    auto actualTemplate = !clipFileTemplate.isEmpty()? clipFileTemplate:
        settings.value("storage/clip-template", DEFAULT_CLIP_TEMPLATE).toString();

    settings.beginGroup("gst");
    recordLimit = duration > 0? duration:
        settings.value("clip-limit", DEFAULT_CLIP_LIMIT).toBool()?
            settings.value("clip-countdown", DEFAULT_CLIP_COUNTDOWN).toInt(): 0;

    if (!activePipeline->recording)
    {
        QString imageExt = getExt(settings.value("image-encoder", DEFAULT_IMAGE_ENCODER).toString());
        auto clipFileName = activePipeline->appendVideoTail(outputPath, "clip", replace(actualTemplate, ++clipNo), false);
        qDebug() << clipFileName;
        if (!clipFileName.isEmpty())
        {
            if (settings.value("save-clip-thumbnails", DEFAULT_SAVE_CLIP_THUMBNAILS).toBool())
            {
                QFileInfo fi(clipFileName);
                clipPreviewFileName = fi.absolutePath()
                    .append(QDir::separator()).append('.').append(fi.fileName()).append(imageExt);
            }
            else
            {
                auto item = new QListWidgetItem(QFileInfo(clipFileName).baseName(), listImagesAndClips);
                item->setToolTip(clipFileName);
                item->setIcon(QIcon(":/buttons/movie"));
                listImagesAndClips->setItemSelected(item, true);
                listImagesAndClips->scrollToItem(item);
            }

            // Until the real clip recording starts, we should disable this button
            //
            btnRecordStart->setEnabled(false);
            activePipeline->recording = true;
            activePipeline->setImageLocation(clipPreviewFileName);
            activePipeline->enableClip(true);
        }
        else
        {
            activePipeline->removeVideoTail("clip");
            QMessageBox::critical(this, windowTitle(),
                tr("Failed to start recording.\nCheck the error log for details."), QMessageBox::Ok);
        }
    }
    else
    {
        // Extend recording time
        //
        countdown = recordLimit;
    }

    sound->play(DATA_FOLDER + "/sound/record.ac3");
    return true;
}

void MainWindow::onRecordStopClick()
{
    Q_FOREACH(auto p, pipelines)
    {
        if (p->recording)
        {
            p->removeVideoTail("clip");
            p->recording = false;
        }
    }

    clipPreviewFileName.clear();
    countdown = 0;
    if (recordTimerId)
    {
        killTimer(recordTimerId);
        recordTimerId = 0;
    }

    btnRecordStop->setEnabled(false);
    updateOverlayText(countdown);
}

void MainWindow::updateStartButton()
{
    QIcon icon(running? ":/buttons/stop": ":/buttons/start");
    QString strOnOff(running? tr("End study"): tr("Start study"));
    btnStart->setIcon(icon);
    auto shortcut = btnStart->shortcut(); // save the shortcut...
    btnStart->setText(strOnOff);
    btnStart->setShortcut(shortcut); // ...and restore

    btnRecordStart->setEnabled(running);
    btnRecordStop->setEnabled(running && activePipeline->recording);
    btnSnapshot->setEnabled(running);
    actionSettings->setDisabled(running);
#ifdef WITH_DICOM
    actionWorklist->setDisabled(running);
    if (worklist)
    {
        worklist->setDisabled(running);
    }
#endif
}

void MainWindow::onPrepareSettingsMenu()
{
    QSettings settings;

    auto menu = static_cast<QMenu*>(sender());
    foreach (auto action, menu->actions())
    {
        auto propName = action->data().toString();
        if (!propName.isEmpty())
        {
            action->setChecked(settings.value(propName).toBool());
            action->setDisabled(running);
        }
    }
}

void MainWindow::toggleSetting()
{
    QSettings settings;
    auto propName = static_cast<QAction*>(sender())->data().toString();
    bool enable = !settings.value(propName).toBool();
    settings.setValue(propName, enable);
    updatePipeline();
}

void MainWindow::onShowAboutClick()
{
    AboutDialog(this).exec();
}

void MainWindow::onShowArchiveClick()
{
#ifndef WITH_TOUCH
    if (archiveWindow == nullptr)
    {
        archiveWindow = new ArchiveWindow();
        archiveWindow->updateRoot();
    }
#endif
    archiveWindow->setPath(outputPath.absolutePath());
#ifdef WITH_TOUCH
    mainStack->slideInWidget(archiveWindow);
#else
    archiveWindow->show();
    archiveWindow->activateWindow();
#endif
}

void MainWindow::onShowSettingsClick()
{
    SettingsDialog dlg(QString(), this);
    connect(&dlg, SIGNAL(apply()), this, SLOT(updatePipeline()));
    if (dlg.exec())
    {
        updatePipeline();
    }
}

void MainWindow::onEnableWidget(QWidget* widget, bool enable)
{
    widget->setEnabled(enable);
}

void MainWindow::updateStartDialog()
{
    if (!accessionNumber.isEmpty())
        dlgPatient->setAccessionNumber(accessionNumber);
    if (!patientId.isEmpty())
        dlgPatient->setPatientId(patientId);
    if (!patientName.isEmpty())
        dlgPatient->setPatientName(patientName);
    if (!patientSex.isEmpty())
        dlgPatient->setPatientSex(patientSex);
    if (!patientBirthDate.isEmpty())
        dlgPatient->setPatientBirthDateStr(patientBirthDate);
    if (!physician.isEmpty())
        dlgPatient->setPhysician(physician);
    if (!studyName.isEmpty())
        dlgPatient->setStudyDescription(studyName);
}

#ifdef WITH_DICOM
void MainWindow::onStartStudy(DcmDataset* patient)
#else
void MainWindow::onStartStudy()
#endif
{
    QWaitCursor wait(this);

    // Switch focus to the main window
    //
    activateWindow();

#ifdef WITH_TOUCH
    mainStack->slideInWidget("Main");
#else
    show();
#endif

    if (running)
    {
        QMessageBox::warning(this, windowTitle(), tr("Failed to start a study.\nAnother study is in progress."));
        return;
    }

    if (dlgPatient)
    {
        updateStartDialog();
        return;
    }

    QSettings settings;
    listImagesAndClips->clear();
    dlgPatient = new PatientDataDialog(false, "start-study", this);

#ifdef WITH_DICOM
    if (patient)
    {
        dlgPatient->readPatientData(patient);
    }
    else
#endif
    {
        updateStartDialog();
    }

    switch (dlgPatient->exec())
    {
#ifdef WITH_DICOM
    case SHOW_WORKLIST_RESULT:
        onShowWorkListClick();
        // passthrouht
#endif
    case QDialog::Rejected:
        delete dlgPatient;
        dlgPatient = nullptr;
        return;
    }

    accessionNumber  = fixFileName(dlgPatient->accessionNumber());
    patientId        = fixFileName(dlgPatient->patientId());
    patientName      = fixFileName(dlgPatient->patientName());
    patientSex       = fixFileName(dlgPatient->patientSex());
    patientBirthDate = fixFileName(dlgPatient->patientBirthDateStr());
    physician        = fixFileName(dlgPatient->physician());
    studyName        = fixFileName(dlgPatient->studyDescription());

    settings.setValue("study-no", ++studyNo);
    updateOutputPath(true);
    if (archiveWindow)
    {
        archiveWindow->setPath(outputPath.absolutePath());
    }

    // After updateOutputPath the outputPath is usable
    //
#ifdef WITH_DICOM
    if (patient)
    {
        // Make a clone
        //
        pendingPatient = new DcmDataset(*patient);
    }
    else
    {
        pendingPatient = new DcmDataset();
    }

    dlgPatient->savePatientData(pendingPatient);

#if __BYTE_ORDER == __LITTLE_ENDIAN
    const E_TransferSyntax writeXfer = EXS_LittleEndianImplicit;
#elif __BYTE_ORDER == __BIG_ENDIAN
    const E_TransferSyntax writeXfer = EXS_BigEndianImplicit;
#else
#error "Unsupported byte order"
#endif

    imageNo = clipNo = 0;
    auto localPatientInfoFile = outputPath.absoluteFilePath(".patient.dcm");
    if (QFileInfo(localPatientInfoFile).exists())
    {
        DcmDataset ds;
        ds.loadFile((const char*)localPatientInfoFile.toLocal8Bit());
        ds.findAndGetUint16(DCM_ImageNo, imageNo);
        ds.findAndGetUint16(DCM_ClipNo, clipNo);
    }

    auto cond = pendingPatient->saveFile((const char*)localPatientInfoFile.toLocal8Bit(), writeXfer);
    if (cond.bad())
    {
        QMessageBox::critical(this, windowTitle(), QString::fromLocal8Bit(cond.text()));
    }
#ifdef Q_OS_WIN
    else
    {
        SetFileAttributesW(localPatientInfoFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
#endif

    settings.beginGroup("dicom");
    if (settings.value("start-with-mpps", true).toBool() && !settings.value("mpps-server").toString().isEmpty())
    {
        DcmClient client(UID_ModalityPerformedProcedureStepSOPClass);
        pendingSOPInstanceUID = client.nCreateRQ(pendingPatient);
        if (pendingSOPInstanceUID.isNull())
        {
            QMessageBox::critical(this, windowTitle(), client.lastError());
        }
        pendingPatient->putAndInsertString(DCM_SOPInstanceUID, pendingSOPInstanceUID.toUtf8());
    }
    settings.endGroup();
#else // WITH_DICOM
    auto localPatientInfoFile = outputPath.absoluteFilePath(".patient");
    QSettings patientData(localPatientInfoFile, QSettings::IniFormat);
    dlgPatient->savePatientData(patientData);
    imageNo = patientData.value("last-image-no", 0).toUInt();
    clipNo  = patientData.value("last-clip-no", 0).toUInt();

#ifdef Q_OS_WIN
    SetFileAttributesW(localPatientInfoFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
#endif // WITH_DICOM

    running = startVideoRecord();
    activePipeline->enableEncoder(running);
    updateOverlayText(countdown);
    updateStartButton();
    updateWindowTitle();
    activePipeline->displayWidget->update();
    delete dlgPatient;
    dlgPatient = nullptr;
}

void MainWindow::onStopStudy()
{
    QSettings settings;
    QWaitCursor wait(this);

    onRecordStopClick();

    Q_FOREACH (auto p, pipelines)
    {
        p->removeVideoTail("video");
    }

    running = false;
    updateOverlayText(countdown);

#ifdef WITH_DICOM
    if (pendingPatient)
    {
        char seriesUID[100] = {0};
        dcmGenerateUniqueIdentifier(seriesUID, SITE_SERIES_UID_ROOT);

        if (!pendingSOPInstanceUID.isEmpty() && settings.value("dicom/complete-with-mpps", true).toBool())
        {
            DcmClient client(UID_ModalityPerformedProcedureStepSOPClass);
            if (!client.nSetRQ(seriesUID, pendingPatient, pendingSOPInstanceUID))
            {
                QMessageBox::critical(this, windowTitle(), client.lastError());
            }
        }

#if __BYTE_ORDER == __LITTLE_ENDIAN
    const E_TransferSyntax writeXfer = EXS_LittleEndianImplicit;
#elif __BYTE_ORDER == __BIG_ENDIAN
    const E_TransferSyntax writeXfer = EXS_BigEndianImplicit;
#else
#error "Unsupported byte order"
#endif

        auto localPatientInfoFile = outputPath.absoluteFilePath(".patient.dcm");
        pendingPatient->putAndInsertUint16(DCM_ImageNo, imageNo);
        pendingPatient->putAndInsertUint16(DCM_ClipNo, clipNo);
        pendingPatient->saveFile((const char*)localPatientInfoFile.toLocal8Bit(), writeXfer);

        delete pendingPatient;
        pendingPatient = nullptr;
    }

    pendingSOPInstanceUID.clear();
#else // WITH_DICOM
    QSettings patientData(outputPath.absoluteFilePath(".patient"), QSettings::IniFormat);
    patientData.setValue("last-image-no", imageNo);
    patientData.setValue("last-clip-no", clipNo);
#endif // WITH_DICOM

    accessionNumber.clear();
    patientId.clear();
    patientName.clear();
    patientSex.clear();
    patientBirthDate.clear();
    if (settings.value("reset-physician").toBool())
    {
        physician.clear();
    }
    if (settings.value("reset-study-name").toBool())
    {
        studyName.clear();
    }

    updateStartButton();

    Q_FOREACH (auto p, pipelines)
    {
        p->enableEncoder(false);
        p->displayWidget->update();
    }

    // Clear the capture list
    //
    listImagesAndClips->clear();
}

void MainWindow::onSwapSources(QWidget* dst)
{
    auto src = static_cast<QWidget*>(sender());

    // Swap with main video widget
    //
    if (dst == nullptr)
    {
        dst = activePipeline->displayWidget;
    }

    if (src == dst)
    {
        // Nothing to do.
        //
        return;
    }

    if (src == activePipeline->displayWidget)
    {
        src = dst;
        dst = activePipeline->displayWidget;
    }

    QSettings settings;
    settings.beginGroup("gst");
    if (dst == activePipeline->displayWidget)
    {
        // Swap alt src with main src
        //
        int idx = layoutSources->indexOf(src);

        layoutVideo->removeWidget(dst);
        layoutSources->removeWidget(src);

        src->setMinimumSize(mainSrcSize);
        src->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layoutVideo->insertWidget(0, src);

        dst->setMinimumSize(altSrcSize);
        dst->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        layoutSources->insertWidget(idx, dst, 0, Qt::AlignTop);
    }
    else
    {
        // Swap alt src with another alt src
        //
        int idxSrc = layoutSources->indexOf(src);
        int idxDst = layoutSources->indexOf(dst);

        layoutSources->removeWidget(src);
        layoutSources->insertWidget(idxDst, src, 0, Qt::AlignTop);
        layoutSources->removeWidget(dst);
        layoutSources->insertWidget(idxSrc, dst, 0, Qt::AlignTop);
    }

    settings.beginWriteArray("src");
    for (int i = 0; i < pipelines.size(); ++i)
    {
        settings.setArrayIndex(i);
        auto idx = layoutSources->indexOf(pipelines[i]->displayWidget);
        settings.setValue("order", idx);
        if (idx < 0)
        {
            activePipeline = pipelines[i];
        }
    }
    settings.endArray();
    settings.endGroup();
}

#ifdef WITH_DICOM
void MainWindow::onShowWorkListClick()
{
#ifndef WITH_TOUCH
    if (worklist == nullptr)
    {
        worklist = new Worklist();
        connect(worklist, SIGNAL(startStudy(DcmDataset*)), this, SLOT(onStartStudy(DcmDataset*)));
    }
    worklist->show();
    worklist->activateWindow();
#else
    mainStack->slideInWidget(worklist);
#endif
}
#endif
