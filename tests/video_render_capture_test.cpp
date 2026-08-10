// 真实 XCB 行为验证：视频内容快照、目标详情浮层证据元数据、四列表契约、
// 结束保留和重置清除。替换已移除的 detectionBox API，不持久化图像，不做像素计数。

#include "MainWindow/MainWindow.h"
#include "MainWindow/TacticalMapWidget.h"
#include "MainWindow/VideoOverlayWidget.h"
#include "MainWindow/VideoStreamPanel.h"
#include "MainWindow/TargetDetailOverlay.h"

#include <QApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QWindow>
#include <QTest>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    window.raise();
    window.activateWindow();

    auto *startButton = window.findChild<QPushButton *>(QStringLiteral("mapToolbarStart"));
    auto *stopButton = window.findChild<QPushButton *>(QStringLiteral("mapToolbarStop"));
    auto *resetButton = window.findChild<QPushButton *>(QStringLiteral("mapToolbarReset"));
    auto *targetTable = window.findChild<QTableWidget *>(QStringLiteral("targetTable"));
    auto *map = window.findChild<TacticalMapWidget *>(QStringLiteral("tacticalMap"));
    auto *videoPanel = window.findChild<VideoStreamPanel *>(QStringLiteral("videoPiP"));
    auto *hudOverlay = videoPanel ? videoPanel->overlay() : nullptr;
    auto *detailOverlay = window.findChild<TargetDetailOverlay *>(QStringLiteral("targetDetailOverlay"));
    auto *evidenceImageLabel = window.findChild<QLabel *>(QStringLiteral("targetDetailEvidenceImage"));

    if (!startButton || !stopButton || !resetButton || !targetTable || !map
        || !videoPanel || !hudOverlay || !detailOverlay
        || !evidenceImageLabel) {
        qCritical() << "[VideoCaptureTest] required widgets missing";
        return 2;
    }

    // 四列契约
    if (targetTable->columnCount() != 4) {
        qCritical() << "[VideoCaptureTest] target table must have 4 columns, got"
                    << targetTable->columnCount();
        return 2;
    }

    // HUD 叠加层必须透明透传鼠标（仅 HUD，不含检测框）
    if (!hudOverlay->testAttribute(Qt::WA_TransparentForMouseEvents)) {
        qCritical() << "[VideoCaptureTest] HUD overlay must have WA_TransparentForMouseEvents";
        return 2;
    }

    auto *appPtr = &app;
    auto *windowPtr = &window;

    QTimer::singleShot(1000, startButton, &QPushButton::click);

    QTimer::singleShot(9000, appPtr, [=]() {
        const bool usingXcb = QGuiApplication::platformName() == QLatin1String("xcb");
        const QWindow *nativeWindow = windowPtr->windowHandle();
        const bool windowExposed = nativeWindow && nativeWindow->isExposed()
                && nativeWindow->screen();

        const bool targetInjected = targetTable->rowCount() > 0;
        const bool mapSynced = map->targetCount() > 0;
        const bool detailHiddenBeforeSelection = !detailOverlay->isVisible();

        // 事件时间快照：XCB 下视频帧应为非空
        const QImage snapshot = videoPanel->currentFrameSnapshot();
        const bool snapshotValid = !snapshot.isNull();

        const bool playbackAdvanced = videoPanel->position() >= 5000;
        const int mapTargetsBeforeEnd = map->targetCount();

        const QTableWidgetItem *targetItem = targetTable->item(0, 0);
        if (!targetItem) {
            qCritical() << "[VideoCaptureTest] target row missing";
            appPtr->exit(3);
            return;
        }
        QTest::mouseClick(targetTable->viewport(), Qt::LeftButton, Qt::NoModifier,
                          targetTable->visualItemRect(targetItem).center());
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        const bool detailVisibleAfterSelection = detailOverlay->isVisible();
        const bool detailIdCorrect = detailOverlay->currentTargetId()
                == QLatin1String("T-001");

        auto *idLabel = windowPtr->findChild<QLabel *>(QStringLiteral("targetDetailIdLabel"));
        auto *captureTimeLabel = windowPtr->findChild<QLabel *>(
            QStringLiteral("targetDetailCaptureTimeValue"));
        auto *videoTimeLabel = windowPtr->findChild<QLabel *>(
            QStringLiteral("targetDetailVideoTimeValue"));
        auto *provenanceLabel = windowPtr->findChild<QLabel *>(
            QStringLiteral("targetDetailProvenanceValue"));

        const bool evidenceMetadataPopulated =
                idLabel && idLabel->text() == QLatin1String("T-001")
                && captureTimeLabel && !captureTimeLabel->text().isEmpty()
                && captureTimeLabel->text() != QLatin1String("-")
                && videoTimeLabel && !videoTimeLabel->text().isEmpty()
                && videoTimeLabel->text() != QLatin1String("-")
                && provenanceLabel && provenanceLabel->text().contains(QStringLiteral("模拟"));

        const bool sidebarSelected = targetTable->currentRow() == 0;

        // 选中后证据图像 pixmap 必须非空（XCB 下视频帧非空 -> 证据快照非空）
        const bool evidenceImagePopulated = evidenceImageLabel->pixmap() != nullptr
                && !evidenceImageLabel->pixmap()->isNull();

        qInfo() << "[VideoCaptureTest] platform=" << QGuiApplication::platformName()
                << "targetInjected=" << targetInjected
                << "mapSynced=" << mapSynced
                << "detailHiddenBeforeSelection=" << detailHiddenBeforeSelection
                << "snapshotValid=" << snapshotValid
                << "playbackAdvanced=" << playbackAdvanced
                << "detailVisibleAfterSelection=" << detailVisibleAfterSelection
                << "detailIdCorrect=" << detailIdCorrect
                << "evidenceMetadataPopulated=" << evidenceMetadataPopulated
                << "evidenceImagePopulated=" << evidenceImagePopulated
                << "sidebarSelected=" << sidebarSelected;

        stopButton->click();

        QTimer::singleShot(250, appPtr, [=]() {
            const bool stoppedAtStart = videoPanel->position() == 0;
            const bool retainedTargets = targetTable->rowCount() > 0
                    && map->targetCount() == mapTargetsBeforeEnd;
            const bool detailRetained = detailOverlay->isVisible();
            const bool frameCleared = !videoPanel->hasFrame();
            // 结束后证据图像 pixmap 必须保留（onStopDetection 不清除证据）
            const bool evidenceImageRetained = evidenceImageLabel->pixmap() != nullptr
                    && !evidenceImageLabel->pixmap()->isNull();

            qInfo() << "[VideoCaptureTest] stoppedAtStart=" << stoppedAtStart
                    << "retainedTargets=" << retainedTargets
                    << "detailRetained=" << detailRetained
                    << "frameCleared=" << frameCleared
                    << "evidenceImageRetained=" << evidenceImageRetained;

            resetButton->click();

            QTimer::singleShot(250, appPtr, [=]() {
                const bool targetsCleared = targetTable->rowCount() == 0
                        && map->targetCount() == 0;
                const bool detailHidden = !detailOverlay->isVisible();
                // 重置后证据图像 pixmap 必须清空（reset -> clearEvidence -> pixmap 清除）
                const bool evidenceImageCleared = evidenceImageLabel->pixmap() == nullptr
                        || evidenceImageLabel->pixmap()->isNull();

                qInfo() << "[VideoCaptureTest] targetsCleared=" << targetsCleared
                        << "detailHidden=" << detailHidden
                        << "evidenceImageCleared=" << evidenceImageCleared;

                const bool allPassed = usingXcb
                        && windowExposed
                        && targetInjected
                        && mapSynced
                        && detailHiddenBeforeSelection
                        && snapshotValid
                        && playbackAdvanced
                        && detailVisibleAfterSelection
                        && detailIdCorrect
                        && evidenceMetadataPopulated
                        && evidenceImagePopulated
                        && sidebarSelected
                        && stoppedAtStart
                        && retainedTargets
                        && detailRetained
                        && frameCleared
                        && evidenceImageRetained
                        && targetsCleared
                        && detailHidden
                        && evidenceImageCleared;

                appPtr->exit(allPassed ? 0 : 1);
            });
        });
    });

    return app.exec();
}
