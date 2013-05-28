#include "aboutdialog.h"
#include "product.h"

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <gst/gst.h>

#ifdef WITH_DICOM
#define HAVE_CONFIG_H
#include <dcmtk/dcmdata/dcuid.h>
#endif

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent)
{
    QBoxLayout* layoutMain = new QHBoxLayout;
    layoutMain->setContentsMargins(16,16,16,16);
    auto icon = new QLabel();
    icon->setPixmap(qApp->windowIcon().pixmap(64));
    layoutMain->addWidget(icon, 1, Qt::AlignTop);
    QBoxLayout* layoutText = new QVBoxLayout;
    layoutText->setContentsMargins(16,16,16,16);

    auto lblTitle = new QLabel(QString(PRODUCT_FULL_NAME).append(" ").append(PRODUCT_VERSION_STR));
    auto titleFont = lblTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() * 2);
    lblTitle->setFont(titleFont);
    layoutText->addWidget(lblTitle);
    layoutText->addSpacing(16);

    layoutText->addWidget(new QLabel(tr("Based on:")));
    auto lblQt = new QLabel(tr("<a href=\"http://qt-project.org/\">Qt ").append(QT_VERSION_STR).append("</a>"));
    lblQt->setOpenExternalLinks(true);
    layoutText->addWidget(lblQt);
    auto lblGstreamer = new QLabel(tr("<a href=\"http://gstreamer.freedesktop.org/\">").append(gst_version_string()).append("</a>"));
    lblGstreamer->setOpenExternalLinks(true);
    layoutText->addWidget(lblGstreamer);
#ifdef WITH_DICOM
    auto lblDcmtk = new QLabel(tr("<a href=\"http://dcmtk.org/\">DCMTK ").append(OFFIS_DCMTK_VERSION_STRING).append("</a>"));
    lblDcmtk->setOpenExternalLinks(true);
    layoutText->addWidget(lblDcmtk);
#endif
    auto lblIconsSimplico = new QLabel(tr("<a href=\"http://neurovit.deviantart.com/art/simplicio-92311415\">Simplicio icon set by Neurovit</a>"));
    lblIconsSimplico->setOpenExternalLinks(true);
    layoutText->addWidget(lblIconsSimplico);
    auto lblIconsWin8 = new QLabel(tr("<a href=\"http://icons8.com/\">Icons8 icon set by VisualPharm</a>"));
    lblIconsWin8->setOpenExternalLinks(true);
    layoutText->addWidget(lblIconsWin8);
    layoutText->addSpacing(16);

    auto lblCopyright = new QLabel(tr("<p>Copyright (C) 2013 <a href=\"%1\">%2</a>. All rights reserved.</p>")
       .arg(PRODUCT_SITE_URL, ORGANIZATION_FULL_NAME));
    lblCopyright->setOpenExternalLinks(true);
    layoutText->addWidget(lblCopyright);
    layoutText->addSpacing(16);

    auto lblWarranty = new QLabel(tr("The program is provided AS IS with NO WARRANTY OF ANY KIND,\n"
                                     "INCLUDING THE WARRANTY OF DESIGN,\n"
                                     "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."));
    layoutText->addWidget(lblWarranty);
    layoutText->addSpacing(16);

    QPushButton* btnClose = new QPushButton(tr("Close"));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(accept()));
    layoutText->addWidget(btnClose, 1, Qt::AlignRight);
    layoutMain->addLayout(layoutText);
    setLayout(layoutMain);
}
