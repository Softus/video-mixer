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

#ifndef WORKLIST_H
#define WORKLIST_H

#include <QDateTime>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QBoxLayout;
class QTableWidget;
class QTableWidgetItem;
class QToolBar;
QT_END_NAMESPACE

class DcmDataset;
class DcmClient;

class Worklist : public QWidget
{
    Q_OBJECT

    // UI
    //
    QTableWidget* table;
    QDateTime     maxDate;
    int           timeColumn;
    int           dateColumn;
    QAction*      actionBack;
    QAction*      actionLoad;
    QAction*      actionDetail;
    QAction*      actionStartStudy;
    QToolBar*     createToolBar();

    // DICOM
    //
    DcmClient* activeConnection;
    QString pendingSOPInstanceUID;

public:
    explicit Worklist(QWidget *parent = 0);

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual void hideEvent(QHideEvent *);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void showEvent(QShowEvent *);

signals:
    void startStudy(DcmDataset* patient);

private slots:
    void onLoadClick();
    void onShowDetailsClick();
    void onStartStudyClick();
    void onItemDoubleClicked(QTableWidgetItem* item);
    void onCurrentItemChanged(QTableWidgetItem*,QTableWidgetItem*);
    void onAddRow(DcmDataset* responseIdentifiers);
    void onBackToMainWindowClick();
};

#endif // WORKLIST_H
