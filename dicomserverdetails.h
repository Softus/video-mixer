#ifndef DICOMSERVERDETAILS_H
#define DICOMSERVERDETAILS_H

#include <QDialog>

class DicomServerDetails : public QDialog
{
    Q_OBJECT
public:
    explicit DicomServerDetails(QWidget *parent = 0);
    
signals:
    
public slots:
    void onClickTest();
};

#endif // DICOMSERVERDETAILS_H
