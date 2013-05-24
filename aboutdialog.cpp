#include "aboutdialog.h"

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent)
{
    QString str = tr("%1 %2\n\nCopyright 2013 %3. All rights reserved.\n\nThe program is provided AS IS with NO WARRANTY OF ANY KIND,\nINCLUDING THE WARRANTY OF DESIGN,\nMERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.")
        .arg(qApp->applicationName(), qApp->applicationVersion(), qApp->organizationDomain());
    QBoxLayout* layoutMain = new QVBoxLayout;
    layoutMain->addWidget(new QLabel(str));
    layoutMain->addSpacing(1);
    QPushButton* btnClose = new QPushButton(tr("Close"));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(accept()));
    layoutMain->addWidget(btnClose);
    setLayout(layoutMain);
}
