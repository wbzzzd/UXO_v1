// 雷达页（GprView）离屏 UI 契约测试（REQ-012 / gpr-display.md §7.3）：
//   1) 稳定对象名存在（GprView/GprSuspectPanel/GprBScanWidget 全部契约对象名）；
//   2) 无效数据路径 → §6.3 空态降级（状态页提示 + 回放控件禁用）；
//   3) 真实样例数据：频段/通道切换信号、首通道异步加载完成（数据缺失自动 QSKIP）；
//   4) 疑似列表确认/误报意图信号经 GprView 转发，且真实数据下处置候选从本地列表移除；
//   5) [模拟] 标注可见（工具栏"模拟数据" + 疑似面板标题 [模拟][GPR自动检测]，§6.4）。

#include "MainWindow/GprView.h"
#include "MainWindow/GprBScanWidget.h"
#include "MainWindow/GprSuspectPanel.h"

#include "GPR/GprThresholdDetector.h"

#include <QtTest/QtTest>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QStackedWidget>
#include <QTableWidget>

class GprViewTest : public QObject
{
    Q_OBJECT

private slots:
    void stableObjectNamesExist();
    void invalidDatasetDegradesToEmpty();
    void realDatasetSwitchAndPlaybackReady();
    void suspectIntentForwarding();
    void simulationTagVisible();
};

void GprViewTest::stableObjectNamesExist()
{
    GprView view;
    QVERIFY2(view.findChild<QComboBox *>(QStringLiteral("gprBandCombo")), "gprBandCombo 缺失");
    QVERIFY2(view.findChild<QComboBox *>(QStringLiteral("gprChannelCombo")), "gprChannelCombo 缺失");
    QVERIFY2(view.findChild<QComboBox *>(QStringLiteral("gprGainModeCombo")), "gprGainModeCombo 缺失");
    QVERIFY2(view.findChild<QSlider *>(QStringLiteral("gprContrastSlider")), "gprContrastSlider 缺失");
    QVERIFY2(view.findChild<QSlider *>(QStringLiteral("gprZoomSlider")), "gprZoomSlider 缺失");
    QVERIFY2(view.findChild<QLabel *>(QStringLiteral("gprSimTagLabel")), "gprSimTagLabel 缺失");
    QVERIFY2(view.findChild<QStackedWidget *>(QStringLiteral("gprCenterStack")), "gprCenterStack 缺失");
    QVERIFY2(view.findChild<QLabel *>(QStringLiteral("gprStateLabel")), "gprStateLabel 缺失");
    QVERIFY2(view.findChild<GprBScanWidget *>(QStringLiteral("gprBScanWidget")), "gprBScanWidget 缺失");
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("gprPlayPauseButton")), "gprPlayPauseButton 缺失");
    QVERIFY2(view.findChild<QComboBox *>(QStringLiteral("gprSpeedCombo")), "gprSpeedCombo 缺失");
    QVERIFY2(view.findChild<QSlider *>(QStringLiteral("gprProgressSlider")), "gprProgressSlider 缺失");
    QVERIFY2(view.findChild<QLabel *>(QStringLiteral("gprPositionLabel")), "gprPositionLabel 缺失");
    QVERIFY2(view.findChild<GprSuspectPanel *>(QStringLiteral("gprSuspectPanel")), "gprSuspectPanel 缺失");
    QVERIFY2(view.findChild<QTableWidget *>(QStringLiteral("gprSuspectTable")), "gprSuspectTable 缺失");
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("gprConfirmButton")), "gprConfirmButton 缺失");
    QVERIFY2(view.findChild<QPushButton *>(QStringLiteral("gprFalsePositiveButton")), "gprFalsePositiveButton 缺失");
}

void GprViewTest::invalidDatasetDegradesToEmpty()
{
    GprView view;
    view.resize(1280, 720);
    view.show();

    // 无效路径 → §6.3 空态：返回 false、状态页可见、回放控件禁用
    QVERIFY(!view.loadDataset(QStringLiteral("/nonexistent/gpr/dataset/path")));
    QVERIFY(!view.hasDataset());

    auto *stateLabel = view.findChild<QLabel *>(QStringLiteral("gprStateLabel"));
    auto *centerStack = view.findChild<QStackedWidget *>(QStringLiteral("gprCenterStack"));
    auto *playPause = view.findChild<QPushButton *>(QStringLiteral("gprPlayPauseButton"));
    auto *progress = view.findChild<QSlider *>(QStringLiteral("gprProgressSlider"));
    auto *speed = view.findChild<QComboBox *>(QStringLiteral("gprSpeedCombo"));
    QVERIFY(centerStack->currentWidget() == stateLabel);
    QVERIFY(!stateLabel->text().isEmpty());
    QVERIFY(!playPause->isEnabled());
    QVERIFY(!progress->isEnabled());
    QVERIFY(!speed->isEnabled());
}

void GprViewTest::realDatasetSwitchAndPlaybackReady()
{
#ifndef GPR_SAMPLE_DATA_DIR
    QSKIP("未定义 GPR_SAMPLE_DATA_DIR，跳过真实数据用例");
#else
    const QDir sampleDir(QStringLiteral(GPR_SAMPLE_DATA_DIR));
    if (!sampleDir.exists()) {
        QSKIP("样例数据目录不存在，跳过真实数据用例");
    }

    GprView view;
    view.resize(1600, 900);
    view.show();

    QSignalSpy bandSpy(&view, &GprView::bandChanged);
    QSignalSpy channelSpy(&view, &GprView::channelChanged);
    QVERIFY(view.loadDataset(sampleDir.absolutePath()));
    QVERIFY(view.hasDataset());

    auto *bandCombo = view.findChild<QComboBox *>(QStringLiteral("gprBandCombo"));
    auto *channelCombo = view.findChild<QComboBox *>(QStringLiteral("gprChannelCombo"));
    auto *playPause = view.findChild<QPushButton *>(QStringLiteral("gprPlayPauseButton"));
    QCOMPARE(bandCombo->count(), 2);      // 样例数据 H/L 两组（§5.4）
    QCOMPARE(channelCombo->count(), 16);  // 16 组 T/R 对

    // 首通道异步加载（校验 + t0 估计 + 阈值检测）完成后回放控件可用；
    // 样例数据位于 /mnt/d（WSL DrvFs），逐道懒加载小读较慢，首载实测约 40s，超时放宽至 120s
    QTRY_VERIFY_WITH_TIMEOUT(playPause->isEnabled(), 120000);

    // 处置流程（§5.3）：确认/误报信号转发后候选从本地列表移除；在首通道（H/T1R1）
    // 上执行（下方切换检查后所在通道候选可能为空，无法覆盖本路径）
    auto *table = view.findChild<QTableWidget *>(QStringLiteral("gprSuspectTable"));
    auto *confirmBtn = view.findChild<QPushButton *>(QStringLiteral("gprConfirmButton"));
    auto *fpBtn = view.findChild<QPushButton *>(QStringLiteral("gprFalsePositiveButton"));
    const QList<GPR::GprSuspect> suspectsBefore = view.suspects();
    QVERIFY2(!suspectsBefore.isEmpty(), "首通道无检测候选，无法执行处置用例");
    QCOMPARE(table->rowCount(), suspectsBefore.size());
    // 平滑轨迹访问器：候选道应能解析出有效 RTK 位置（目标注入定位源）
    QVERIFY(view.trackPointForTrace(suspectsBefore.first().traceIndex).valid);

    QSignalSpy confirmSpy(&view, &GprView::suspectConfirmRequested);
    QSignalSpy fpSpy(&view, &GprView::suspectFalsePositiveRequested);

    table->selectRow(0);
    QTRY_VERIFY_WITH_TIMEOUT(confirmBtn->isEnabled(), 2000);
    confirmBtn->click();
    QCOMPARE(confirmSpy.count(), 1);
    QCOMPARE(confirmSpy.first().at(0).toInt(), suspectsBefore.first().id);
    QCOMPARE(view.suspects().size(), suspectsBefore.size() - 1);
    QCOMPARE(table->rowCount(), suspectsBefore.size() - 1);

    table->selectRow(0);   // 首候选已移除，首行现为另一候选
    QTRY_VERIFY_WITH_TIMEOUT(fpBtn->isEnabled(), 2000);
    fpBtn->click();
    QCOMPARE(fpSpy.count(), 1);
    QCOMPARE(view.suspects().size(), suspectsBefore.size() - 2);
    QCOMPARE(table->rowCount(), suspectsBefore.size() - 2);

    // 切换频段 → bandChanged；切换通道 → channelChanged（均携带非空 id）
    const int bandBefore = bandSpy.count();
    bandCombo->setCurrentIndex(1);
    QTRY_COMPARE_WITH_TIMEOUT(bandSpy.count(), bandBefore + 1, 5000);
    QVERIFY(!bandSpy.last().at(0).toString().isEmpty());

    const int channelBefore = channelSpy.count();
    channelCombo->setCurrentIndex(1);
    QTRY_COMPARE_WITH_TIMEOUT(channelSpy.count(), channelBefore + 1, 5000);
    QVERIFY(!channelSpy.last().at(0).toString().isEmpty());
#endif
}

void GprViewTest::suspectIntentForwarding()
{
    GprView view;
    view.resize(1600, 900);
    view.show();

    auto *panel = view.findChild<GprSuspectPanel *>(QStringLiteral("gprSuspectPanel"));
    auto *table = view.findChild<QTableWidget *>(QStringLiteral("gprSuspectTable"));
    auto *confirmBtn = view.findChild<QPushButton *>(QStringLiteral("gprConfirmButton"));
    auto *fpBtn = view.findChild<QPushButton *>(QStringLiteral("gprFalsePositiveButton"));

    // 空列表 → 禁用态（§6.3）
    panel->setSuspects({});
    QVERIFY(!confirmBtn->isEnabled());
    QVERIFY(!fpBtn->isEnabled());

    // 构造两条候选（展示用途，数值无物理意义）
    GPR::GprSuspect s1;
    s1.id = 1; s1.traceIndex = 100; s1.sampleIndex = 60; s1.depthM = 0.42;
    s1.amplitude = 1234.5; s1.traceSpan = 12; s1.sampleSpan = 8;
    GPR::GprSuspect s2;
    s2.id = 2; s2.traceIndex = 250; s2.sampleIndex = 75; s2.depthM = 0.55;
    s2.amplitude = 987.0; s2.traceSpan = 9; s2.sampleSpan = 6;

    panel->setSuspects({s1, s2});
    QCOMPARE(table->rowCount(), 2);
    QVERIFY(!confirmBtn->isEnabled());   // 未选中仍禁用
    QVERIFY(!fpBtn->isEnabled());

    QSignalSpy confirmSpy(&view, &GprView::suspectConfirmRequested);
    QSignalSpy fpSpy(&view, &GprView::suspectFalsePositiveRequested);

    table->selectRow(0);
    QTRY_VERIFY_WITH_TIMEOUT(confirmBtn->isEnabled(), 2000);
    confirmBtn->click();
    QCOMPARE(confirmSpy.count(), 1);
    QCOMPARE(confirmSpy.first().at(0).toInt(), 1);

    table->selectRow(1);
    QTRY_VERIFY_WITH_TIMEOUT(fpBtn->isEnabled(), 2000);
    fpBtn->click();
    QCOMPARE(fpSpy.count(), 1);
    QCOMPARE(fpSpy.first().at(0).toInt(), 2);
}

void GprViewTest::simulationTagVisible()
{
    GprView view;

    // 工具栏"模拟数据"标注（§6.4）
    auto *simTag = view.findChild<QLabel *>(QStringLiteral("gprSimTagLabel"));
    QVERIFY(simTag->text().contains(QStringLiteral("模拟")));

    // 疑似面板标题含 [模拟][GPR自动检测] 标注（§6.4）
    auto *panel = view.findChild<GprSuspectPanel *>(QStringLiteral("gprSuspectPanel"));
    const auto labels = panel->findChildren<QLabel *>();
    bool tagged = false;
    for (const QLabel *label : labels) {
        if (label->text().contains(QStringLiteral("模拟"))
            && label->text().contains(QStringLiteral("GPR"))) {
            tagged = true;
            break;
        }
    }
    QVERIFY2(tagged, "疑似面板标题缺少 [模拟][GPR自动检测] 标注");
}

QTEST_MAIN(GprViewTest)
#include "gpr_view_test.moc"
