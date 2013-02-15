#include "mainwindow.h"
#include "QBoxLayout"
#include "QPushButton"
#include "QFrame"

static const QSize minSize(96,96);

QPushButton* MainWindow::createButton(const char *slot)
{
    auto btn = new QPushButton();
    btn->setMinimumSize(minSize);
    btn->setIconSize(minSize);
    connect(btn, SIGNAL(clicked()), this, slot);

    return btn;
}

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    recordAll(false),
    running(true),
    recording(true)
{
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

    outputLayout->addWidget(new QFrame());
    auto pic = new QFrame();
    outputLayout->addWidget(pic);


    auto mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(outputLayout);

    setLayout(mainLayout);
    setBackgroundRole(QPalette::Dark);
}

MainWindow::~MainWindow()
{
}


void MainWindow::onStartClick()
{
    running = !running;
    QIcon icon(running? ":/buttons/stop": ":/buttons/add");
    QString strOnOff(running? tr("Stop\nexamination"): tr("Start\nexamination"));
    btnStart->setIcon(icon);
    btnStart->setText(strOnOff);
}

void MainWindow::onSnapshotClick()
{

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
