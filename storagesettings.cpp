/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "storagesettings.h"
#include "qwaitcursor.h"
#include "defaults.h"

#include <QHBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>

StorageSettings::StorageSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    QFormLayout* layoutMain = new QFormLayout;

    QHBoxLayout* layoutPath = new QHBoxLayout;
    textOutputPath = new QLineEdit(settings.value("output-path", DEFAULT_OUTPUT_PATH).toString());
    QPushButton* browseButton = new QPushButton(QString(0x2026));
    browseButton->setMaximumWidth(32);
    connect(browseButton, SIGNAL(clicked()), this, SLOT(onClickBrowse()));
    QLabel* lblPath = new QLabel(tr("&Output path"));
    lblPath->setBuddy(textOutputPath);
    layoutPath->addWidget(lblPath);
    layoutPath->addWidget(textOutputPath, 1);
    layoutPath->addWidget(browseButton);
    layoutMain->addRow(layoutPath);

    QFrame *frameFolder = new QFrame;
    frameFolder->setFrameShape(QFrame::Box);
    frameFolder->setFrameShadow(QFrame::Sunken);
    QFormLayout* layoutFolder = new QFormLayout;
    textFolderTemplate = new QLineEdit(settings.value("folder-template", DEFAULT_FOLDER_TEMPLATE).toString());
    layoutFolder->addRow(tr("&Folder template"), textFolderTemplate);
    frameFolder->setLayout(layoutFolder);
    layoutMain->addRow(frameFolder);

    QFrame *frameFile = new QFrame;
    frameFile->setFrameShape(QFrame::Box);
    frameFile->setFrameShadow(QFrame::Sunken);
    QFormLayout* fileLayout = new QFormLayout;
    fileLayout->addRow(tr("&Pictures template"), textImageTemplate = new QLineEdit(settings.value("image-template", DEFAULT_IMAGE_TEMPLATE).toString()));
    fileLayout->addRow(tr("&Clips template"), textClipTemplate = new QLineEdit(settings.value("clip-template", DEFAULT_CLIP_TEMPLATE).toString()));
    fileLayout->addRow(tr("&Video template"), textVideoTemplate = new QLineEdit(settings.value("video-template", DEFAULT_VIDEO_TEMPLATE).toString()));
    frameFile->setLayout(fileLayout);
    layoutMain->addRow(frameFile);
    layoutMain->addRow(new QLabel(tr("%yyyy%\t\tyear\n%mm%\t\tmonth\n%dd%\t\tday\n%hh%\t\thour\n%min%\t\tminute\n"
                                     "%id%\t\tpatient id, if specified\n%name%\tpatient name, if specified\n"
                                     "%physician%\tphysician name, if specified\n%study%\tstudy name\n"
                                     "%nn%\t\tsequential number")));
    setLayout(layoutMain);
}

void StorageSettings::onClickBrowse()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    dlg.selectFile(textOutputPath->text());
    if (dlg.exec())
    {
        textOutputPath->setText(dlg.selectedFiles().at(0));
    }
}

void StorageSettings::save()
{
    QSettings settings;
    settings.setValue("output-path", textOutputPath->text());
    settings.setValue("folder-template", textFolderTemplate->text());
    settings.setValue("image-template", textImageTemplate->text());
    settings.setValue("clip-template", textClipTemplate->text());
    settings.setValue("video-template", textVideoTemplate->text());
}
