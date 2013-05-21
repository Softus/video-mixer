#include "dicomserversettings.h"
#include "dicomserverdetails.h"
#include "qwaitcursor.h"
#include <QBoxLayout>
#include <QListView>
#include <QPushButton>

DicomServerSettings::DicomServerSettings(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(4,0,4,0);
    mainLayout->addWidget(new QListView);

    QBoxLayout* buttonsLayout = new QVBoxLayout;
    QPushButton* btnAdd = new QPushButton(tr("&Add"));
    connect(btnAdd, SIGNAL(clicked()), this, SLOT(onClickAdd()));
    buttonsLayout->addWidget(btnAdd);
    buttonsLayout->addWidget(new QPushButton(tr("&Edit")));
    buttonsLayout->addWidget(new QPushButton(tr("&Remove")));
    buttonsLayout->addStretch(1);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

void DicomServerSettings::onClickAdd()
{
    QWaitCursor(this);
    DicomServerDetails dlg(this);
    if (dlg.exec())
    {
    }
}
