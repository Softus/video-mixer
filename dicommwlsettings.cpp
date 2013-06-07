#include "dicommwlsettings.h"
#include "comboboxwithpopupsignal.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QSettings>

DicomMwlSettings::DicomMwlSettings(QWidget *parent) :
    QWidget(parent)
{
    auto mainLayout = new QFormLayout;
    mainLayout->addRow(checkUseMwl = new QCheckBox(tr("MWL &Function")), cbMwlServer = new ComboBoxWithPopupSignal);
    mainLayout->addRow(checkStartWithMwl = new QCheckBox(tr("&Start examination based on the worklist")));
    setLayout(mainLayout);

    QSettings settings;
    QString mwlServer = settings.value("mwl-server").toString();
    if (!mwlServer.isEmpty())
    {
        cbMwlServer->addItem(mwlServer);
        cbMwlServer->setCurrentIndex(0);
        checkUseMwl->setChecked(true);
    }
    checkStartWithMwl->setChecked(settings.value("start-with-mwl", true).toBool());

    connect(cbMwlServer, SIGNAL(currentIndexChanged(int)), this, SLOT(onServerChanged(int)));
    connect(cbMwlServer, SIGNAL(beforePopup()), this, SLOT(onUpdateServers()));
}

void DicomMwlSettings::onUpdateServers()
{
    QSettings settings;
    settings.beginGroup("servers");
    auto server = cbMwlServer->currentText();
    cbMwlServer->clear();
    cbMwlServer->addItems(settings.allKeys());
    settings.endGroup();
    cbMwlServer->setCurrentIndex(cbMwlServer->findText(server));
}

void DicomMwlSettings::onServerChanged(int idx)
{
    checkUseMwl->setChecked(idx >= 0);
}

void DicomMwlSettings::save()
{
    QSettings settings;
    settings.setValue("mwl-server", checkUseMwl->isChecked()? cbMwlServer->currentText(): nullptr);
    settings.setValue("start-with-mwl", checkStartWithMwl->isChecked());
}
