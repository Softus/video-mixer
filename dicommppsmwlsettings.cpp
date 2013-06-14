#include "dicommppsmwlsettings.h"
#include "comboboxwithpopupsignal.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QSettings>

DicomMppsMwlSettings::DicomMppsMwlSettings(QWidget *parent) :
    QWidget(parent)
{
    auto mainLayout = new QFormLayout;
    mainLayout->addRow(checkUseMwl = new QCheckBox(tr("MWL &Function")), cbMwlServer = new ComboBoxWithPopupSignal);

    QSettings settings;
    QString mwlServer = settings.value("mwl-server").toString();
    if (!mwlServer.isEmpty())
    {
        cbMwlServer->addItem(mwlServer);
        cbMwlServer->setCurrentIndex(0);
        checkUseMwl->setChecked(true);
    }

    connect(cbMwlServer, SIGNAL(currentIndexChanged(int)), this, SLOT(onServerChanged(int)));
    connect(cbMwlServer, SIGNAL(beforePopup()), this, SLOT(onUpdateServers()));

    mainLayout->addRow(checkTransCyr = new QCheckBox(tr("&Translate latin letters back to cyrillic")));
    checkTransCyr->setChecked(settings.value("translate-cyrillic", true).toBool());

    auto spacer = new QWidget;
    spacer->setMinimumHeight(16);
    mainLayout->addWidget(spacer);

    mainLayout->addRow(checkUseMpps = new QCheckBox(tr("MPPS F&unction")), cbMppsServer = new ComboBoxWithPopupSignal);
    mainLayout->addRow(checkStartWithMpps = new QCheckBox(tr("&In progress is automatically sent when an examitaion has been started")));
    mainLayout->addRow(checkCompleteWithMpps = new QCheckBox(tr("&Completed is automatically sent when an examitaion has been ended")));

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

    setLayout(mainLayout);
}

void DicomMppsMwlSettings::onUpdateServers()
{
    auto cb = static_cast<QComboBox*>(sender());
    QStringList serverNames = parent()->property("DICOM servers").toStringList();

    if (serverNames.isEmpty())
    {
        QSettings settings;
        settings.beginGroup("servers");
        serverNames = settings.allKeys();
        settings.endGroup();
    }

    auto server = cb->currentText();
    cb->clear();
    cb->addItems(serverNames);
    cb->setCurrentIndex(cb->findText(server));
}

void DicomMppsMwlSettings::onServerChanged(int idx)
{
    auto cb = static_cast<QComboBox*>(sender());
    if (cb == cbMppsServer)
    {
        checkUseMpps->setChecked(idx >= 0);
    }
    else
    {
        checkUseMwl->setChecked(idx >= 0);
    }
}

void DicomMppsMwlSettings::save()
{
    QSettings settings;
    settings.setValue("mwl-server", checkUseMwl->isChecked()? cbMwlServer->currentText(): nullptr);
    settings.setValue("mpps-server", checkUseMpps->isChecked()? cbMppsServer->currentText(): nullptr);
    settings.setValue("start-with-mpps", checkStartWithMpps->isChecked());
    settings.setValue("complete-with-mpps", checkCompleteWithMpps->isChecked());
    settings.setValue("translate-cyrillic", checkTransCyr->isChecked());
}
