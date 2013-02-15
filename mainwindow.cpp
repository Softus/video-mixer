#include "mainwindow.h"
#include <QApplication>
#include <QFrame>
#include <QBoxLayout>
#include <QPushButton>
#include <QSettings>

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/Parse>
#include <QGlib/Type>

static const QSize minSize(96,96);

QPushButton* MainWindow::createButton(const char *slot)
{
    auto btn = new QPushButton();
    btn->setMinimumSize(minSize);
    btn->setIconSize(minSize);
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}

static void Dump(QGst::ElementPtr elm)
{
    if (!elm)
    {
        qCritical() << " (null) ";
    }

    foreach(auto prop,  elm->listProperties())
    {
        const QString n = prop->name();
        const QGlib::Value v = elm->property(n.toUtf8());
        qCritical() << n << " = " << v.get<QString>();
    }

    QGst::ChildProxyPtr childProxy =  elm.dynamicCast<QGst::ChildProxy>();
    if (childProxy)
    {
        childProxy->data(NULL);
        auto cnt = childProxy->childrenCount();
        for (uint i = 0; i < cnt; ++i)
        {
            qCritical() << "==== CHILD ==== " << i;
            Dump(childProxy->childByIndex(i).dynamicCast<QGst::Element>());
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    recordAll(false),
    running(true),
    recording(true)
{
    const QStringList& args = qApp->arguments();
    const QString strSrc = args.length() > 1? args.at(1): "v4l2src";
    const QString strEnc = args.length() > 2? args.at(2): "mpeg2enc";
    const QString strMux = args.length() > 3? args.at(3): "mpegtsmux";

    QString pipeDefault = QString("%1 ! video/x-raw-yuv,width=640,height=480 ! tee name=splitter \
                                 ! queue ! autovideoconvert ! clockoverlay halign=left valign=top time-format=\"%Y/%m/%d %H:%M:%S\" ! timeoverlay halign=right valign=top ! ximagesink splitter. \
                                 ! queue ! videoscale ! video/x-raw-yuv,width=160,height=120 ! %2 ! %3 ! filesink location=/video/ex%4.mpg"
                                 ).arg(strSrc).arg(strEnc).arg(strMux)
                                .arg(QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate));
    QSettings settings;
    QString pipeDef = settings.value("pipeline", pipeDefault).toString();

    pipeline = QGst::Parse::launch(pipeDef).dynamicCast<QGst::Pipeline>();
    if (pipeline)
    {
        QGlib::connect(pipeline->bus(), "message::error", this, &MainWindow::onBusMessage);
        pipeline->bus()->addSignalWatch();
        Dump(pipeline);
    }

    auto buttonsLayout = new QHBoxLayout();

    btnStart = createButton(SLOT(onStartClick()));
    onStartClick();
    buttonsLayout->addWidget(btnStart);

    auto btnSnapshot = createButton(SLOT(onSnapshotClick()));
    btnSnapshot->setIcon(QIcon(":/buttons/camera"));
    btnSnapshot->setText(tr("Take\nsnapshot"));
    buttonsLayout->addWidget(btnSnapshot);

    btnRecord = createButton(SLOT(onRecordClick()));
    onRecordClick();
    buttonsLayout->addWidget(btnRecord);

    buttonsLayout->addSpacing(200);
    btnRecordAll = createButton(SLOT(onRecordAllClick()));

    btnRecordAll->setFlat(true);
    btnRecordAll->setFocusPolicy(Qt::NoFocus);
    onRecordAllClick();
    buttonsLayout->addWidget(btnRecordAll);

    auto outputLayout = new QHBoxLayout();

    videoOut = new QGst::Ui::VideoWidget();
    videoOut->setMinimumSize(minSize);
    outputLayout->addWidget(videoOut);
    auto pic = new QFrame();
    outputLayout->addWidget(pic);


    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);

    setLayout(mainLayout);
    videoOut->watchPipeline(pipeline);
}

MainWindow::~MainWindow()
{
    pipeline->setState(QGst::StateNull);
}


void MainWindow::onBusMessage(const QGst::MessagePtr & message)
{
    switch (message->type()) {
    case QGst::MessageEos:
        qCritical() << "EOS???";
        break;
    case QGst::MessageError:
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        break;
    default:
        break;
    }
}

void MainWindow::onStartClick()
{
    running = !running;
    QIcon icon(running? ":/buttons/stop": ":/buttons/add");
    QString strOnOff(running? tr("Stop\nexamination"): tr("Start\nexamination"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);

    // start recording
    //
    pipeline->setState(running? QGst::StatePlaying: QGst::StateNull);
}

void MainWindow::onSnapshotClick()
{
    Dump(pipeline);
}

void MainWindow::onRecordClick()
{
    recording = !recording;
    QIcon icon(recording? ":/buttons/pause": ":/buttons/play");
    QString strOnOff(recording? tr("Pause"): tr("Record"));
    btnRecord->setIcon(icon);
    btnRecord->setText(strOnOff);
}

void MainWindow::onRecordAllClick()
{
    QIcon icon(":/buttons/record_on");

    recordAll = !recordAll;
    QString strOnOff(recordAll? tr("on"): tr("off"));
    btnRecordAll->setIcon(icon.pixmap(minSize, recordAll? QIcon::Normal: QIcon::Disabled));
    btnRecordAll->setToolTip(tr("Recording of entire examination is %1").arg(strOnOff));
    btnRecordAll->setText(tr("Record\nis %1").arg(strOnOff));
}
