#ifndef DEBUGSETTINGS_H
#define DEBUGSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGroupBox;
class QxtLineEdit;
QT_END_NAMESPACE

class DebugSettings : public QWidget
{
    Q_OBJECT
    QGroupBox   *grpGst;
    QComboBox   *cbGstLogLevel;
    QxtLineEdit *textGstOutput;
    QCheckBox   *checkGstNoColor;
    QxtLineEdit *textGstDot;
    QxtLineEdit *textGstDebug;

#ifdef WITH_DICOM
    QGroupBox   *grpDicom;
    QComboBox   *cbDicomLogLevel;
    QxtLineEdit *textDicomOutput;
    QxtLineEdit *textDicomConfig;
#endif

public:
    Q_INVOKABLE explicit DebugSettings(QWidget *parent = 0);

public slots:
    void save();
private slots:
    void onClickBrowse();
};

#endif // DEBUGSETTINGS_H
