#include "videoeditor.h"

#include <QAction>
#include <QBoxLayout>
#include <QDebug>
#include <QSettings>
#include <QxtSpanSlider>
#include <QToolBar>
#include <QToolButton>

#include <QGst/Ui/VideoWidget>

static QSize videoSize(352, 258);

static void DamnQtMadeMeDoTheSunsetByHands(QToolBar* bar)
{
    foreach (auto action, bar->actions())
    {
        auto shortcut = action->shortcut();
        if (shortcut.isEmpty())
        {
            continue;
        }
        action->setToolTip(action->text() + " (" + shortcut.toString(QKeySequence::NativeText) + ")");
    }
}

VideoEditor::VideoEditor(QWidget *parent) :
    QWidget(parent)
{
    auto layoutMain = new QVBoxLayout;

    auto barEditor = new QToolBar(tr("Video editor"));
    barEditor->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto actionBack = barEditor->addAction(QIcon(":buttons/back"), tr("Exit"), this, SLOT(close()));
    actionBack->setShortcut(QKeySequence::Close);

    auto spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    barEditor->addWidget(spacer);

    auto actionSave = barEditor->addAction(QIcon(":buttons/back"), tr("Save"), this, SLOT(close()));
    actionSave->setShortcut(QKeySequence::Save);
    auto actionSaveAs = barEditor->addAction(QIcon(":buttons/back"), tr("Save as").append(0x2026), this, SLOT(close()));
    actionSaveAs->setShortcut(QKeySequence::SaveAs);

#ifndef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif

    auto videoWidget = new QGst::Ui::VideoWidget;
    layoutMain->addWidget(videoWidget);
    videoWidget->setMinimumSize(videoSize);
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto sliderPos = new QSlider(Qt::Horizontal);
    layoutMain->addWidget(sliderPos);
    auto sliderRange = new QxtSpanSlider(Qt::Horizontal);
    sliderRange->setTickPosition(QSlider::TicksAbove);
    sliderRange->setSpan(20, 80);
    layoutMain->addWidget(sliderRange);
#ifdef WITH_TOUCH
    layoutMain->addWidget(barEditor);
#endif
    setLayout(layoutMain);

    QSettings settings;
    restoreGeometry(settings.value("video-editor-geometry").toByteArray());
    setWindowState((Qt::WindowState)settings.value("video-editor-state").toInt());

    DamnQtMadeMeDoTheSunsetByHands(barEditor);
}

void VideoEditor::closeEvent(QCloseEvent *evt)
{
    QSettings settings;
    settings.setValue("video-editor-geometry", saveGeometry());
    settings.setValue("video-editor-state", (int)windowState() & ~Qt::WindowMinimized);
    QWidget::closeEvent(evt);
}

void VideoEditor::loadFile(const QString& filePath)
{

}
