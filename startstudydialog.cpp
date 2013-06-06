#include "startstudydialog.h"
#include "worklist.h"

#include <QFrame>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QToolBox>

StartStudyDialog::StartStudyDialog(Worklist* worklist, QWidget *parent) :
    QDialog(parent),
    worklist(worklist)
{
    auto layout = new QVBoxLayout();
    auto toolbox = new QToolBox();
    toolbox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    toolbox->addItem(worklist, tr("Worklist"));
    auto frm = new QFrame();
    auto form = new QFormLayout();
    form->addRow(tr("ID"), new QTextEdit());
    form->addRow(tr("Name"), new QTextEdit());
    form->addRow(tr("Sex"), new QTextEdit());
    form->addRow(tr("Birthdate"), new QTextEdit());
    frm->setLayout(form);
    frm->setMaximumSize(QSize(320, 160));
    toolbox->addItem(frm, tr("New patient"));
    toolbox->setMaximumSize(32000, 32000);
    toolbox->setMinimumSize(800, 600);
    layout->addWidget(toolbox, Qt::AlignJustify);

    auto btnStart = new QPushButton(tr("Start"));
    btnStart->setDefault(true);
    connect(btnStart, SIGNAL(clicked()), this, SLOT(accept()));
    layout->addWidget(btnStart);
    setLayout(layout);
}

void StartStudyDialog::done(int result)
{
    // Detach the worklist shared with the main window
    //
    worklist->setParent(nullptr);
    QDialog::done(result);
}

void StartStudyDialog::savePatientFile(const QString& outputPath)
{
}
