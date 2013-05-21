#include "rtpsettings.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>

RtpSettings::RtpSettings(QWidget *parent) :
    QWidget(parent)
{
    QFormLayout* layout = new QFormLayout;

    auto textRtpClients = new QLineEdit;
    layout->addRow(tr("&RTP clients"), textRtpClients);
    layout->addRow(nullptr, new QCheckBox(tr("&Enable RTP")));

    setLayout(layout);
}
