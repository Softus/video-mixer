/*
 * Copyright (C) 2013-2015 Irkutsk Diagnostic Center.
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

#include "debug.h"
#include "../defaults.h"
#include "../qwaitcursor.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QxtLineEdit>

#include <gst/gstinfo.h>

#ifdef WITH_DICOM
#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/oflog/logger.h>
namespace dcmtk{}
using namespace dcmtk;
#endif

DebugSettings::DebugSettings(QWidget *parent) :
    QWidget(parent)
{
    QSettings settings;
    settings.beginGroup("debug");
    auto layoutMain = new QVBoxLayout;

    layoutMain->addWidget(grpGst = new QGroupBox(tr("Enable &GStreamer debugging")));
    grpGst->setCheckable(true);
    grpGst->setChecked(settings.value("gst-debug-on", DEFAULT_GST_DEBUG_ON).toBool());
    auto layoutGst = new QFormLayout;

    layoutGst->addRow(tr("&Log level"), cbGstLogLevel = new QComboBox);
    auto gstDebugLevel = (GstDebugLevel)settings.value("gst-debug-level", DEFAULT_GST_DEBUG_LEVEL).toInt();
    for (int i = GST_LEVEL_ERROR; i < GST_LEVEL_COUNT; ++i)
    {
        auto name = QString::fromUtf8(gst_debug_level_get_name((GstDebugLevel)i));
        if (name.isEmpty())
        {
            // For some reason, GstDebugLevel contains gaps
            //
            continue;
        }

        cbGstLogLevel->addItem(name, i);
        if (gstDebugLevel == (GstDebugLevel)i)
        {
            cbGstLogLevel->setCurrentIndex(cbGstLogLevel->count() - 1);
        }
    }

    layoutGst->addRow(tr("&Output file"), textGstOutput = new QxtLineEdit());
    textGstOutput->setButtonIcon(QIcon(":/buttons/folder"));
    textGstOutput->setButtonPosition(QxtLineEdit::OuterRight);
    textGstOutput->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    textGstOutput->setText(settings.value("gst-debug-log-file", DEFAULT_GST_DEBUG_LOG_FILE).toString());
    connect(textGstOutput, SIGNAL(buttonClicked()), this, SLOT(onClickBrowse()));

    layoutGst->addRow(nullptr, checkGstNoColor = new QCheckBox(tr("Ou&tput no color")));
    checkGstNoColor->setChecked(settings.value("gst-debug-no-color", DEFAULT_GST_DEBUG_NO_COLOR).toBool());

    layoutGst->addRow(tr("&Pipelines dump folder"), textGstDot = new QxtLineEdit());
    textGstDot->setButtonIcon(QIcon(":/buttons/folder"));
    textGstDot->setButtonPosition(QxtLineEdit::OuterRight);
    textGstDot->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    textGstDot->setText(settings.value("gst-debug-dot-dir", DEFAULT_GST_DEBUG_DOT_DIR).toString());
    connect(textGstDot, SIGNAL(buttonClicked()), this, SLOT(onClickBrowse()));

    layoutGst->addRow(tr("&Categories"), textGstDebug = new QxtLineEdit());
    textGstDebug->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    textGstDebug->setText(settings.value("gst-debug", DEFAULT_GST_DEBUG).toString());
    textGstDebug->setSampleText(tr("Comma separated list"));

    grpGst->setLayout(layoutGst);

#ifdef WITH_DICOM
    log4cplus::LogLevel levels[] = {
        log4cplus::FATAL_LOG_LEVEL,
        log4cplus::ERROR_LOG_LEVEL,
        log4cplus::WARN_LOG_LEVEL,
        log4cplus::INFO_LOG_LEVEL,
        log4cplus::DEBUG_LOG_LEVEL,
        log4cplus::TRACE_LOG_LEVEL,
    };

    layoutMain->addWidget(grpDicom = new QGroupBox(tr("Enable &DICOM debugging")));
    grpDicom->setCheckable(true);
    grpDicom->setChecked(settings.value("dcmtk-debug-on", DEFAULT_DCMTK_DEBUG_ON).toBool());
    auto layoutDicom = new QFormLayout;

    layoutDicom->addRow(tr("Log l&evel"), cbDicomLogLevel = new QComboBox);
    auto dicomDebugLevel = settings.value("dcmtk-log-level", DEFAULT_DCMTK_DEBUG_LEVEL).toString();
    for (size_t i = 0; i < sizeof(levels)/sizeof(levels[0]); ++i)
    {
        auto name = QString::fromStdString(log4cplus::getLogLevelManager().toString(levels[i]).c_str());
        cbDicomLogLevel->addItem(name);
        if (dicomDebugLevel == name)
        {
            cbDicomLogLevel->setCurrentIndex(cbDicomLogLevel->count() - 1);
        }
    }

    layoutDicom->addRow(tr("O&utput file"), textDicomOutput = new QxtLineEdit());
    textDicomOutput->setButtonIcon(QIcon(":/buttons/folder"));
    textDicomOutput->setButtonPosition(QxtLineEdit::OuterRight);
    textDicomOutput->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    textDicomOutput->setText(settings.value("dcmtk-log-file", DEFAULT_DCMTK_DEBUG_LOG_FILE).toString());
    connect(textDicomOutput, SIGNAL(buttonClicked()), this, SLOT(onClickBrowse()));

    layoutDicom->addRow(tr("Co&nfig file"), textDicomConfig = new QxtLineEdit());
    textDicomConfig->setButtonIcon(QIcon(":/buttons/folder"));
    textDicomConfig->setButtonPosition(QxtLineEdit::OuterRight);
    textDicomConfig->setResetButtonMode(QxtLineEdit::ShowResetNotEmpty);
    textDicomConfig->setText(settings.value("dcmtk-log-config", DEFAULT_DCMTK_LOG_CONFIG_FILE).toString());
    connect(textDicomConfig, SIGNAL(buttonClicked()), this, SLOT(onClickBrowse()));

    grpDicom->setLayout(layoutDicom);
#endif

    layoutMain->addStretch();
    layoutMain->addWidget(new QLabel(tr("NOTE: some changes on this page will take effect the next time the application starts.")));
    setLayout(layoutMain);
}

void DebugSettings::onClickBrowse()
{
    QWaitCursor wait(this);
    QFileDialog dlg(this);
    auto textEdit = static_cast<QxtLineEdit*>(sender());
    dlg.setFileMode(textEdit == textGstDot? QFileDialog::DirectoryOnly: QFileDialog::AnyFile);
    dlg.selectFile(textEdit->text());
    if (dlg.exec())
    {
        textEdit->setText(dlg.selectedFiles().first());
    }
}

extern void setupGstDebug(const QSettings& settings);
#ifdef WITH_DICOM
extern void setupDcmtkDebug(const QSettings& settings);
#endif

void DebugSettings::save(QSettings& settings)
{
    settings.beginGroup("debug");

    settings.setValue("gst-debug-on", grpGst->isChecked());
    settings.setValue("gst-debug-level", cbGstLogLevel->itemData(cbGstLogLevel->currentIndex()).toInt());
    settings.setValue("gst-debug-log-file", textGstOutput->text());
    settings.setValue("gst-debug-no-color", checkGstNoColor->isChecked());
    settings.setValue("gst-debug-dot-dir", textGstDot->text());
    settings.setValue("gst-debug", textGstDebug->text());
    setupGstDebug(settings);

#ifdef WITH_DICOM
    settings.setValue("dcmtk-debug-on", grpDicom->isChecked());
    settings.setValue("dcmtk-log-level", cbDicomLogLevel->currentText());
    settings.setValue("dcmtk-log-file", textDicomOutput->text());
    settings.setValue("dcmtk-log-config", textDicomConfig->text());
    setupDcmtkDebug(settings);
#endif
    settings.endGroup();
}
