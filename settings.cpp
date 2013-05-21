/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "settings.h"
#include "rtpsettings.h"
#include "storagesettings.h"
#include "studiessettings.h"
#include "videosettings.h"

#ifdef WITH_DICOM
#include "dicomdevicesettings.h"
#include "dicomserversettings.h"
#include "dicommwlsettings.h"
#include "dicommppssettings.h"
#include "dicomstoragesettings.h"
#include "worklistcolumnsettings.h"
#include "worklistquerysettings.h"
#endif

Q_DECLARE_METATYPE(QMetaObject)
static int QMetaObjectMetaType = qRegisterMetaType<QMetaObject>();

Settings::Settings()
{
    listWidget = new QListWidget;
    listWidget->setMovement(QListView::Static);
    listWidget->setMaximumWidth(200);
    listWidget->setSpacing(2);

    pagesWidget = new QStackedWidget;
    pagesWidget->setMinimumSize(520, 360);

    createPages();

    QHBoxLayout *layoutContent = new QHBoxLayout;
    layoutContent->addWidget(listWidget);
    layoutContent->addWidget(pagesWidget, 1);

    QHBoxLayout *layoutBtns = new QHBoxLayout;
    layoutBtns->addStretch(1);

    QPushButton *btnApply = new QPushButton(tr("Appl&y"));
    connect(btnApply, SIGNAL(clicked()), this, SLOT(onClickApply()));
    layoutBtns->addWidget(btnApply);
    QPushButton *btnCancel = new QPushButton(tr("Cancel"));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    layoutBtns->addWidget(btnCancel);
    QPushButton *btnOk = new QPushButton(tr("Ok"));
    connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
    btnOk->setDefault(true);
    layoutBtns->addWidget(btnOk);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(layoutContent);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(layoutBtns);
    setLayout(mainLayout);

    setWindowTitle(tr("Settings"));
}

void Settings::createPage(const QString& title, const QMetaObject& page)
{
    auto btn= new QListWidgetItem(title, listWidget);
    btn->setData(Qt::UserRole, QVariant::fromValue(page));
}

void Settings::createPages()
{
    createPage(tr("RTP broadcast"), RtpSettings::staticMetaObject);
    createPage(tr("Video source"), VideoSettings::staticMetaObject);
    createPage(tr("Storage"), StorageSettings::staticMetaObject);
    createPage(tr("Studies"), StudiesSettings::staticMetaObject);

#ifdef WITH_DICOM
    createPage(tr("DICOM device"), DicomDeviceSettings::staticMetaObject);
    createPage(tr("DICOM MPPS"), DicomMppsSettings::staticMetaObject);
    createPage(tr("DICOM MWL"), DicomMwlSettings::staticMetaObject);
    createPage(tr("DICOM servers"), DicomServerSettings::staticMetaObject);
    createPage(tr("DICOM storage"), DicomStorageSettings::staticMetaObject);
    createPage(tr("Worklist column display"), WorklistColumnSettings::staticMetaObject);
    createPage(tr("Worklist query settings"), WorklistQuerySettings::staticMetaObject);
#endif

    // Sort pages using current locale
    //
    listWidget->sortItems();

    connect(listWidget,
        SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
        this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
}

void Settings::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    const QVariant& data = current->data(Qt::UserRole);
    if (data.type() == QVariant::UserType)
    {
        const QMetaObject& mobj = data.value<QMetaObject>();
        QWidget* page = static_cast<QWidget*>(mobj.newInstance());
        auto count = pagesWidget->count();
        current->setData(Qt::UserRole, count);
        pagesWidget->addWidget(page);
        pagesWidget->setCurrentIndex(count);
        connect(this, SIGNAL(save()), page, SLOT(save()));
    }
    else
    {
        pagesWidget->setCurrentIndex(current->data(Qt::UserRole).toInt());
    }
}

void Settings::onClickApply()
{
    save();
}

void Settings::accept()
{
    save();
    QDialog::accept();
}
