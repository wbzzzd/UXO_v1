// MOS P0 UI 闭合测试：生成器浮点合同、安全措辞与 controller-owned 导出。

#include "MainWindow/MainWindow.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosPlanningController.h"
#include "Core/MOS/MosTypes.h"

#include <QtTest>
#include <QAbstractButton>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>

class MosUiClosureTest : public QObject
{
    Q_OBJECT
private slots:
    void craterRadiusControlsPreserveFractions();
    void standoffLabelRejectsRealSafetyClaim();
    void generatorResizesAfterFourKTransition();
    void exportButtonUsesControllerCanonicalSnapshot();
    void exportFixtureRejectsWhenNoCommittedResult();
    void exportFixtureRejectsRelativePath();
    void exportFixtureRejectsWrongSuffix();
    void exportFixtureRejectsSymlinkTarget();
    void exportFixtureRejectsNonRegularTarget();
    void exportFixtureWritesValidAbsoluteJsonAndLeavesStateUnchanged();
    void kLabelHasNoForcedBreakAt1280x720();
};

void MosUiClosureTest::craterRadiusControlsPreserveFractions()
{
    // Given: MainWindow 中已构造的生成器模态。
    MainWindow window;
    auto *dialog = window.findChild<MosGeneratorDialog *>(QStringLiteral("DEC-GEN-MODAL"));
    QVERIFY(dialog != nullptr);
    auto *minimum = dialog->findChild<QDoubleSpinBox *>(QStringLiteral("DEC-GEN-CRATER-RMIN"));
    auto *maximum = dialog->findChild<QDoubleSpinBox *>(QStringLiteral("DEC-GEN-CRATER-RMAX"));
    QVERIFY(minimum != nullptr); QVERIFY(maximum != nullptr);
    QCOMPARE(minimum->minimum(), 0.1); QCOMPARE(maximum->maximum(), 100.0);

    // When: 输入合同允许的小数半径。
    minimum->setValue(3.25); maximum->setValue(6.75);

    // Then: currentParams 不发生整数截断。
    const auto values = dialog->currentParams();
    QCOMPARE(values.craterRMin, 3.25); QCOMPARE(values.craterRMax, 6.75);
}

void MosUiClosureTest::standoffLabelRejectsRealSafetyClaim()
{
    // Given: 已构造的 MOS 参数面板标签集合。
    MainWindow window;
    const auto labels = window.findChildren<QLabel *>();

    // When: 检索可见参数措辞。
    bool hasSyntheticLabel = false;
    bool hasUnsafeLabel = false;
    for (const auto *label : labels) {
        QString visibleText = label->text();
        visibleText.remove(QChar('\n'));
        hasSyntheticLabel |= visibleText.contains(
            QStringLiteral("合成 standoff 系数 K（非真实安全参数）"));
        hasUnsafeLabel |= label->text() == QStringLiteral("安全系数 K");
    }

    // Then: 仅保留明确的非真实安全参数语义。
    QVERIFY(hasSyntheticLabel); QVERIFY(!hasUnsafeLabel);
}

void MosUiClosureTest::kLabelHasNoForcedBreakAt1280x720()
{
    // Given: 1280x720 最小支持视口下的 MainWindow 与 K 参数标签。
    MainWindow window;
    window.resize(1280, 720);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *kLabel = window.findChild<QLabel *>(QStringLiteral("DEC-CE-PARAM-K-LABEL"));
    QVERIFY(kLabel != nullptr);

    // When: 检查标签文本与高度。
    // Then: 文本不包含强制换行（由 wordWrap 自然处理），单行高度不被人为撑高。
    QVERIFY(!kLabel->text().contains(QChar('\n')));
    QVERIFY(kLabel->wordWrap());
}

void MosUiClosureTest::generatorResizesAfterFourKTransition()
{
    // Given: DecisionView 先按 4K 视口放大生成器模态。
    MainWindow window;
    window.resize(3840, 2160);
    window.show();
    auto *decisionNav = window.findChild<QAbstractButton *>(QStringLiteral("DEC-NAV-03"));
    QVERIFY(decisionNav != nullptr);
    QTest::mouseClick(decisionNav, Qt::LeftButton);
    QCoreApplication::processEvents();
    auto *dialog = window.findChild<MosGeneratorDialog *>(QStringLiteral("DEC-GEN-MODAL"));
    QVERIFY(dialog != nullptr);
    QVERIFY(dialog->minimumWidth() > 520);

    // When: 同一窗口回到最小支持视口。
    window.resize(1280, 720);
    QCoreApplication::processEvents();

    // Then: minimum 先降低，resize 能恢复 1x 参考尺寸。
    QCOMPARE(dialog->minimumSize(), QSize(520, 360));
    QCOMPARE(dialog->size(), QSize(520, 360));
}

void MosUiClosureTest::exportButtonUsesControllerCanonicalSnapshot()
{
    // Given: 已提交 seed=42 快照与临时导出目录。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    auto *dialog = window.findChild<MosGeneratorDialog *>(QStringLiteral("DEC-GEN-MODAL"));
    auto *button = dialog ? dialog->findChild<QPushButton *>(QStringLiteral("DEC-GEN-JSON")) : nullptr;
    QVERIFY(controller != nullptr); QVERIFY(dialog != nullptr); QVERIFY(button != nullptr);
    const auto before = controller->snapshot();
    QVERIFY(before.hasResult);
    const QByteArray expected = Core::MOS::serializeObstacleSetBytes(before.obstacles);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString originalPath = QDir::currentPath();
    QVERIFY(QDir::setCurrent(directory.path()));

    // When: 通过真实 DEC-GEN-JSON 按钮触发完整信号链。
    button->click();
    QVERIFY(QDir::setCurrent(originalPath));

    // Then: controller 写出 canonical bytes，且权威会话完全不变。
    QFile file(directory.path() + QStringLiteral("/mos-sim-scenario-seed42-prototype.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), expected);
    const auto after = controller->snapshot();
    QCOMPARE(after.committedRevision, before.committedRevision);
    QCOMPARE(after.hasResult, before.hasResult);
    QCOMPARE(after.selectedTier, before.selectedTier);
    QCOMPARE(after.logEntries.size(), before.logEntries.size());
    QCOMPARE(Core::MOS::serializeObstacleSetBytes(after.obstacles), expected);
}

void MosUiClosureTest::exportFixtureRejectsWhenNoCommittedResult()
{
    // Given: 未提交任何 replan 的独立控制器与一个合法的绝对 .json 目标。
    Core::MOS::MosPlanningController controller;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString target = QDir(directory.path()).absoluteFilePath(QStringLiteral("no-commit.json"));

    // When: 直接请求导出。
    const auto result = controller.exportFixture(target);

    // Then: 因无已接受的提交快照，导出在写入前被拒，目标文件不落盘。
    QVERIFY(!result.success);
    QVERIFY(!QFile::exists(target));
}

void MosUiClosureTest::exportFixtureRejectsRelativePath()
{
    // Given: MainWindow 控制器已有 seed-42 提交快照；切到临时 CWD 防止误写。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    QVERIFY(controller != nullptr);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString originalPath = QDir::currentPath();
    QVERIFY(QDir::setCurrent(directory.path()));

    // When: 提供相对路径。
    const auto result = controller->exportFixture(QStringLiteral("relative.json"));

    QVERIFY(QDir::setCurrent(originalPath));

    // Then: 路径策略拒绝，目标不落盘。
    QVERIFY(!result.success);
    QVERIFY(!QFile::exists(directory.path() + QStringLiteral("/relative.json")));
}

void MosUiClosureTest::exportFixtureRejectsWrongSuffix()
{
    // Given: 已有提交快照的控制器与一个绝对 .txt 路径。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    QVERIFY(controller != nullptr);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString target = QDir(directory.path()).absoluteFilePath(QStringLiteral("not-json.txt"));

    // When: 请求导出至非 .json 路径。
    const auto result = controller->exportFixture(target);

    // Then: 后缀策略拒绝，目标不落盘。
    QVERIFY(!result.success);
    QVERIFY(!QFile::exists(target));
}

void MosUiClosureTest::exportFixtureRejectsSymlinkTarget()
{
    // Given: 已有提交快照的控制器与一个指向真实文件的符号链接 .json 目标。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    QVERIFY(controller != nullptr);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString realFile = QDir(directory.path()).absoluteFilePath(QStringLiteral("real.json"));
    const QString linkFile = QDir(directory.path()).absoluteFilePath(QStringLiteral("link.json"));
    {
        QFile f(realFile);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("dummy");
        f.close();
    }
    QVERIFY(QFile::link(realFile, linkFile));

    // When: 请求导出至符号链接路径。
    const auto result = controller->exportFixture(linkFile);

    // Then: 符号链接策略拒绝，真实文件内容不被写穿。
    QVERIFY(!result.success);
    QFile f(realFile);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("dummy"));
    f.close();
}

void MosUiClosureTest::exportFixtureRejectsNonRegularTarget()
{
    // Given: 已有提交快照的控制器与一个目录形态的 .json 目标。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    QVERIFY(controller != nullptr);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString dirTarget = QDir(directory.path()).absoluteFilePath(QStringLiteral("subdir.json"));
    QVERIFY(QDir().mkdir(dirTarget));

    // When: 请求导出至目录路径。
    const auto result = controller->exportFixture(dirTarget);

    // Then: 非普通文件策略拒绝，目录保持空。
    QVERIFY(!result.success);
    QVERIFY(QDir(dirTarget).entryList(QDir::NoDotAndDotDot).isEmpty());
}

void MosUiClosureTest::exportFixtureWritesValidAbsoluteJsonAndLeavesStateUnchanged()
{
    // Given: 已有 seed-42 提交快照的控制器与一个合法的绝对 .json 目标。
    MainWindow window;
    auto *controller = window.findChild<Core::MOS::MosPlanningController *>(
        QStringLiteral("mosPlanningController"));
    QVERIFY(controller != nullptr);
    const auto before = controller->snapshot();
    QVERIFY(before.hasResult);
    QVERIFY(before.result.accepted);
    const QByteArray expected = Core::MOS::serializeObstacleSetBytes(before.obstacles);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString target = QDir(directory.path()).absoluteFilePath(QStringLiteral("valid.json"));

    // When: 直接调用 exportFixture 写入合法绝对路径。
    const auto result = controller->exportFixture(target);

    // Then: 写出 canonical 字节，权威会话状态/revision/日志/障碍物完全不变。
    QVERIFY(result.success);
    QFile file(target);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), expected);
    file.close();
    const auto after = controller->snapshot();
    QCOMPARE(after.committedRevision, before.committedRevision);
    QCOMPARE(after.hasResult, before.hasResult);
    QCOMPARE(after.selectedTier, before.selectedTier);
    QCOMPARE(after.logEntries.size(), before.logEntries.size());
    QCOMPARE(Core::MOS::serializeObstacleSetBytes(after.obstacles), expected);
}

QTEST_MAIN(MosUiClosureTest)
#include "mos_ui_closure_test.moc"