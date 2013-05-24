#include "aboutdialog.h"

#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent)
{
    QBoxLayout* layoutMain = new QVBoxLayout;
    layoutMain->addWidget(new QLabel(tr("Berillyum 1.0 \nBuilt on Apr 01 2013 at 23:29:42\nCopyright 2013 Irkutsk Diagnostic Centre. All rights reserved. \nThe program is provided AS IS with NO WARRANTY OF ANY KIND,\nINCLUDING THE WARRANTY OF DESIGN,\nMERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.")));
    layoutMain->addSpacing(1);
    QPushButton* btnClose = new QPushButton(tr("Close"));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(accept()));
    layoutMain->addWidget(btnClose);
    setLayout(layoutMain);
}
