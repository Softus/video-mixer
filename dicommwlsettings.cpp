#include "dicommwlsettings.h"
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSettings>

DicomMwlSettings::DicomMwlSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QString mwlServer = settings.value("mwl-server").toString();

    QFormLayout* mainLayout = new QFormLayout;
    cbMwlServer = new QComboBox;
    settings.beginGroup("servers");
    cbMwlServer->addItems(settings.allKeys());
    settings.endGroup();
    auto idx = cbMwlServer->findText(mwlServer);
    cbMwlServer->setCurrentIndex(idx);

    checkUseMwl = new QCheckBox(tr("MWL &Function"));
    checkUseMwl->setChecked(idx >= 0);

    mainLayout->addRow(checkUseMwl, cbMwlServer);
    mainLayout->addRow(checkStartWithWL = new QCheckBox(tr("&Start examination based on the worklist")));
    checkStartWithWL->setChecked(settings.value("start-with-mwl", true).toBool());
    setLayout(mainLayout);
}

void DicomMwlSettings::save()
{
    QSettings settings;
    settings.setValue("mwl-server", checkUseMwl->isChecked()? cbMwlServer->currentText(): nullptr);
    settings.setValue("start-with-mwl", checkStartWithWL->isChecked());
}
