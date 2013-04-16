#ifndef DETAILSDIALOG_H
#define DETAILSDIALOG_H

#include <QDialog>

class DcmDataset;

class DetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DetailsDialog(DcmDataset* ds, QWidget* parent = 0);

signals:

public slots:
};

#endif // DETAILSDIALOG_H
