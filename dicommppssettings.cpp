#include "dicommppssettings.h"
#include "comboboxwithpopupsignal.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QSettings>

DicomMppsSettings::DicomMppsSettings(QWidget *parent) :
    QWidget(parent)
{
    auto mainLayout = new QFormLayout;
    mainLayout->addRow(checkUseMpps = new QCheckBox(tr("MPPS &Function")), cbMppsServer = new ComboBoxWithPopupSignal);
    mainLayout->addRow(checkStartWithMpps = new QCheckBox(tr("&In progress is automatically sent when an examitaion has been started")));
    mainLayout->addRow(checkCompleteWithMpps = new QCheckBox(tr("&Completed is automatically sent when an examitaion has been ended")));
    setLayout(mainLayout);

    QSettings settings;
    QString mppsServer = settings.value("mpps-server").toString();
    if (!mppsServer.isEmpty())
    {
        cbMppsServer->addItem(mppsServer);
        cbMppsServer->setCurrentIndex(0);
        checkUseMpps->setChecked(true);
    }
    checkStartWithMpps->setChecked(settings.value("start-with-mpps", true).toBool());
    checkCompleteWithMpps->setChecked(settings.value("complete-with-mpps", true).toBool());

    connect(cbMppsServer, SIGNAL(currentIndexChanged(int)), this, SLOT(onServerChanged(int)));
    connect(cbMppsServer, SIGNAL(beforePopup()), this, SLOT(onUpdateServers()));
}

void DicomMppsSettings::onUpdateServers()
{
    QSettings settings;
    settings.beginGroup("servers");
    auto server = cbMppsServer->currentText();
    cbMppsServer->clear();
    cbMppsServer->addItems(settings.allKeys());
    settings.endGroup();
    cbMppsServer->setCurrentIndex(cbMppsServer->findText(server));
}

void DicomMppsSettings::onServerChanged(int idx)
{
    checkUseMpps->setChecked(idx >= 0);
}

void DicomMppsSettings::save()
{
    QSettings settings;
    settings.setValue("mpps-server", checkUseMpps->isChecked()? cbMppsServer->currentText(): nullptr);
    settings.setValue("start-with-mpps", checkStartWithMpps->isChecked());
    settings.setValue("complete-with-mpps", checkCompleteWithMpps->isChecked());
}
