#ifndef STARTSTUDYDIALOG_H
#define STARTSTUDYDIALOG_H

#include <QDialog>

class Worklist;

class StartStudyDialog : public QDialog
{
    Q_OBJECT

    Worklist*     worklist;

public:
    explicit StartStudyDialog(Worklist* worklist, QWidget *parent = 0);
    void savePatientFile(const QString& outputPath);

protected:
    virtual void done(int);
signals:
    
public slots:
};

#endif // STARTSTUDYDIALOG_H
