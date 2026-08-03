// 巡检员 worker：单轮随机点击 + 自洽性检查。
// 独立进程运行，每轮由 run_loop.sh 启动，崩溃/卡死由外层 timeout 检测。
// 设计原则：只读检查，不修改任何业务代码；复用现有测试的离屏启动方式。
//
// 检查规则（共32条，均为自洽性检查，不依赖业务知识）：
//   R1 三处状态显示一致：选中目标后 目标表/操作面板/决策面板 三处状态相同
//   R2 按钮启用匹配状态：confirm↔Detected, start↔Confirmed, complete↔Disposing
//   R3 未选目标时三按钮全禁用
//   R4 状态转移合法性：点击后状态只能保持或前进一步（不可跳级/倒退）
//   R5 幂等：重复选同一目标，状态不变且日志不增
//   R6 日志最后状态变更的新状态 = 界面当前状态
//   R7 日志增量每动作最多1行
//   R8 日志格式一致：非占位行含 "->" 或 "操作被拒绝" 或 "已选择目标"
//   R9 面板目标ID存在于目标表
//   R10 点击目标行N后，面板显示该行的目标ID
//   R11 切换标签页后，QTabWidget 的 currentIndex 必须等于目标索引
//   R12 刷新按钮不应丢失已选目标（panelTargetId 和 panelStatus 保持）
//   R13 相机操作不应改变模拟状态机或日志
//   R14 任何动作后主窗口仍可见（未意外关闭）
//   R15 视图切换（面板隐藏/显示）后恢复，状态不丢失
//   R16 搜索框输入不应改变选中状态
//   R17 任务/设备表行点击不应改变目标选中状态
//   R18 状态子标签页点击不应改变目标选中状态
//   R19 键盘导航（Tab/Enter/Esc/方向键）不应破坏模拟状态机或丢失选中
//   R20 菜单关闭延迟应 <=300ms（Esc 关闭下拉菜单的耗时）
//   R21 状态机按钮响应应 <=200ms（confirm/start/complete 点击+事件处理耗时）
//   R22 标签页切换响应应 <=200ms（setCurrentIndex+事件处理耗时）
//   R23 搜索框输入过滤响应应 <=200ms（textChanged->onSearchTextChanged 同步过滤耗时）
//   R24 模态对话框弹出时标题应非空（捕获 tr() 缺失或编码异常）
//   R25 对话框关闭后不应残留模态状态（activeModalWidget 应为 null）
//   R26 超长/对抗性输入过滤响应应 <=500ms（主线程不应被同步过滤长时间阻塞）
//   R27 标签页切换后当前页 currentWidget 应非空且可见（捕获页面损坏/空页/隐藏页 bug）
//   R28 非状态机动作不应改变 panelStatus（捕获副作用 bug，如刷新触发状态重置、搜索触发误迁移）
//   R29 会话级 widget 子对象数不应显著增长（孤儿对话框/定时器累积未清理，阈值 +30）
//   R30 启用的状态机按钮点击后必须触发状态前进（捕获 enabled 但 no-op 的按钮 bug，补齐 R4 盲区）
//   R31 Tab 键后焦点应非空且变化（捕获焦点陷阱/死循环/焦点丢失，仅 KeyTab 生效）
//   R32 会话级响应时间退化趋势（前20% vs 后20%平均耗时，比值>2 且 绝对差>50ms，A5 性能守护）

#include "MainWindow/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QAction>
#include <QElapsedTimer>
#include <QHash>
#include <QRandomGenerator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTest>
#include <QTextStream>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>

#include <memory>
#include <vector>

// ===== 工具函数（复用 simulation_workflow_ui_test.cpp 的成熟实现）=====

// 按对象名查找子控件
template <typename WidgetType>
WidgetType *findWidget(MainWindow &window, const char *objectName)
{
    return window.findChild<WidgetType *>(QString::fromLatin1(objectName));
}

// 抓取主窗口现场截图，用于问题取证
void captureScreenshot(MainWindow &window, const QString &dir, const QString &fileName)
{
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QDir().mkpath(dir);
    window.grab().save(QDir(dir).filePath(fileName), "PNG");
}

// ===== 状态提取 =====
// 把不同位置的中文状态文字统一映射成状态码，便于跨控件比较。
// 目标表显示"[模拟] 已确认"，操作面板显示"模拟状态：已确认"，
// 决策面板显示"[模拟] 目标状态：已确认"--都含"已确认"关键词。

QString normalizeStatusText(const QString &text)
{
    if (text.contains(QStringLiteral("未选择"))) {
        return QStringLiteral("None");
    }
    if (text.contains(QStringLiteral("已发现"))) {
        return QStringLiteral("Detected");
    }
    if (text.contains(QStringLiteral("已确认"))) {
        return QStringLiteral("Confirmed");
    }
    if (text.contains(QStringLiteral("处置中"))) {
        return QStringLiteral("Disposing");
    }
    if (text.contains(QStringLiteral("已完成"))) {
        return QStringLiteral("Disposed");
    }
    return QStringLiteral("Unknown");
}

// 目标表第0行的模拟状态（当前只有一个演示目标）
QString getTableStatus(MainWindow &window)
{
    auto *table = findWidget<QTableWidget>(window, "targetTable");
    if (table == nullptr || table->rowCount() == 0) {
        return QStringLiteral("None");
    }
    // 找"状态"列
    for (int col = 0; col < table->columnCount(); ++col) {
        const QTableWidgetItem *header = table->horizontalHeaderItem(col);
        if (header != nullptr && header->text().contains(QStringLiteral("状态"))) {
            const QTableWidgetItem *item = table->item(0, col);
            if (item != nullptr) {
                return normalizeStatusText(item->text());
            }
        }
    }
    return QStringLiteral("None");
}

// 操作面板状态标签
QString getPanelStatus(MainWindow &window)
{
    auto *label = findWidget<QLabel>(window, "simulationStatusLabel");
    if (label == nullptr) {
        return QStringLiteral("None");
    }
    return normalizeStatusText(label->text());
}

// 决策面板状态标签
QString getDecisionStatus(MainWindow &window)
{
    auto *label = findWidget<QLabel>(window, "decisionSimulationStatusLabel");
    if (label == nullptr) {
        return QStringLiteral("None");
    }
    return normalizeStatusText(label->text());
}

// 操作面板当前目标ID（"模拟目标：target-demo-001" -> "target-demo-001"）
QString getPanelTargetId(MainWindow &window)
{
    auto *label = findWidget<QLabel>(window, "simulationTargetLabel");
    if (label == nullptr) {
        return QString();
    }
    const QString text = label->text();
    const int idx = text.indexOf(QStringLiteral("："));
    if (idx < 0) {
        return QString();
    }
    return text.mid(idx + 1).trimmed();
}

// 操作日志全文
QString getLogText(MainWindow &window)
{
    auto *log = findWidget<QWidget>(window, "simulationOperationLog");
    if (log == nullptr) {
        return QString();
    }
    if (auto *te = qobject_cast<QTextEdit *>(log)) {
        return te->toPlainText();
    }
    if (auto *pte = qobject_cast<QPlainTextEdit *>(log)) {
        return pte->toPlainText();
    }
    return QString();
}

// 日志非空行数
int getLogLineCount(MainWindow &window)
{
    const QString text = getLogText(window);
    if (text.isEmpty()) {
        return 0;
    }
    return text.split(QLatin1Char('\n'), Qt::SkipEmptyParts).size();
}

// 按文本查找子控件中的 QPushButton（相机/刷新按钮无 objectName，只能按文本回退）
QPushButton *findButtonByText(MainWindow &window, const QString &text)
{
    const auto buttons = window.findChildren<QPushButton *>();
    for (auto *btn : buttons) {
        if (btn->text().contains(text)) {
            return btn;
        }
    }
    return nullptr;
}

// 查找主窗口内的 QTabWidget（LeftPanelWidget 的 目标/任务/设备 标签页）
QTabWidget *findTabWidget(MainWindow &window)
{
    return window.findChild<QTabWidget *>();
}

// ===== Layer 1 扩展：控件发现与模态对话框处理 =====

// 菜单 action 三元组：指针、所属菜单名、action 文本
struct MenuActionInfo {
    QAction *action;
    QString menuName;
    QString actionText;
};

// 收集菜单栏所有非 checkable、非"退出"的 QAction。
// "退出"会关闭主窗口，必须跳过；checkable 的视图项交给 ViewToggle。
QVector<MenuActionInfo> collectMenuActions(MainWindow &window)
{
    QVector<MenuActionInfo> result;
    auto *bar = window.menuBar();
    if (bar == nullptr) {
        return result;
    }
    for (QAction *menuAction : bar->actions()) {
        QMenu *menu = menuAction->menu();
        if (menu == nullptr) {
            continue;
        }
        const QString menuName = menuAction->text();
        for (QAction *a : menu->actions()) {
            if (a == nullptr || a->isSeparator()) {
                continue;
            }
            if (a->isCheckable()) {
                continue;
            }
            if (a->text().contains(QStringLiteral("退出"))) {
                continue;
            }
            result.append({a, menuName, a->text()});
        }
    }
    return result;
}

// 收集"视图"菜单的 checkable QAction（显示/隐藏面板），用于 ViewToggle
QVector<QAction *> collectViewToggleActions(MainWindow &window)
{
    QVector<QAction *> result;
    auto *bar = window.menuBar();
    if (bar == nullptr) {
        return result;
    }
    for (QAction *menuAction : bar->actions()) {
        QMenu *menu = menuAction->menu();
        if (menu == nullptr) {
            continue;
        }
        if (!menuAction->text().contains(QStringLiteral("视图"))) {
            continue;
        }
        for (QAction *a : menu->actions()) {
            if (a != nullptr && a->isCheckable()) {
                result.append(a);
            }
        }
    }
    return result;
}

// 查找搜索框（无 objectName，按 placeholderText "搜索目标" 查找）
QLineEdit *findSearchEdit(MainWindow &window)
{
    const auto edits = window.findChildren<QLineEdit *>();
    for (auto *edit : edits) {
        if (edit->placeholderText().contains(QStringLiteral("搜索目标"))) {
            return edit;
        }
    }
    return nullptr;
}

// 查找导航按钮（无 objectName，按 "navIndex" 属性查找）
QVector<QPushButton *> findNavButtons(MainWindow &window)
{
    QVector<QPushButton *> result;
    const auto buttons = window.findChildren<QPushButton *>();
    for (auto *btn : buttons) {
        if (btn->property("navIndex").isValid()) {
            result.append(btn);
        }
    }
    return result;
}

// 在触发可能弹模态对话框的 action 前，预设自动关闭定时器。
// QDialog::exec 启动嵌套事件循环，QTimer 在其中仍会触发，从而自动关闭对话框。
// 同时在 50ms 时抓取对话框标题用于 R24 检查（exec 弹出后标题已就绪）。
void scheduleModalDialogAutoClose(QString *capturedTitle, bool *dialogShown,
                                  int delayMs = 300)
{
    // 50ms 抓取标题（exec 已弹出对话框，标题可读）
    QTimer::singleShot(50, qApp, [capturedTitle, dialogShown]() {
        QWidget *modal = QApplication::activeModalWidget();
        if (modal != nullptr) {
            if (capturedTitle != nullptr) {
                *capturedTitle = modal->windowTitle();
            }
            if (dialogShown != nullptr) {
                *dialogShown = true;
            }
        }
    });
    // delayMs 关闭对话框
    QTimer::singleShot(delayMs, qApp, []() {
        QWidget *modal = QApplication::activeModalWidget();
        if (modal != nullptr) {
            modal->close();
        }
    });
}

// ===== Layer 3 扩展：任务/设备表与状态子标签页 =====

// 目标表有 objectName="targetTable"；任务表和设备表无 objectName。
// 通过 QTabWidget 的页序定位：index 1=任务表，index 2=设备表。
QTableWidget *findMissionTable(MainWindow &window)
{
    auto *tabs = findTabWidget(window);
    if (tabs == nullptr || tabs->count() < 2) {
        return nullptr;
    }
    return qobject_cast<QTableWidget *>(tabs->widget(1));
}

QTableWidget *findDeviceTable(MainWindow &window)
{
    auto *tabs = findTabWidget(window);
    if (tabs == nullptr || tabs->count() < 3) {
        return nullptr;
    }
    return qobject_cast<QTableWidget *>(tabs->widget(2));
}

// 状态子标签页 3 个按钮，按文本"待处置任务"/"处置中任务"/"已完成任务"查找
QVector<QPushButton *> findStatusTabButtons(MainWindow &window)
{
    QVector<QPushButton *> result;
    const auto buttons = window.findChildren<QPushButton *>();
    for (auto *btn : buttons) {
        const QString text = btn->text();
        if (text.contains(QStringLiteral("待处置任务"))
            || text.contains(QStringLiteral("处置中任务"))
            || text.contains(QStringLiteral("已完成任务"))) {
            result.append(btn);
        }
    }
    return result;
}

// ===== 检查规则 =====

struct Issue {
    QString type;       // inconsistency / transition_violation / log_mismatch / id_mismatch
    QString rule;       // 规则描述
    QString action;     // 触发问题前执行的动作
    QString details;    // 问题细节
    QString screenshot; // 截图文件名
};

// 点击前后的状态快照，供转移合法性与增量规则对比
struct Snapshot {
    QString panelStatus;
    QString panelTargetId;
    int logLineCount = 0;
    int tabIndex = -1;
};

Snapshot captureSnapshot(MainWindow &window)
{
    Snapshot s;
    s.panelStatus = getPanelStatus(window);
    s.panelTargetId = getPanelTargetId(window);
    s.logLineCount = getLogLineCount(window);
    auto *tabs = findTabWidget(window);
    if (tabs != nullptr) {
        s.tabIndex = tabs->currentIndex();
    }
    return s;
}

// R1：三处状态显示必须一致（仅在选择目标后适用）
void checkStatusConsistency(MainWindow &window, const QString &action,
                            const QString &screenshotDir, int issueIdx,
                            std::vector<Issue> &issues)
{
    const QString panelStatus = getPanelStatus(window);
    if (panelStatus == QStringLiteral("None")) {
        return;
    }
    const QString tableStatus = getTableStatus(window);
    const QString decisionStatus = getDecisionStatus(window);

    if (tableStatus != panelStatus || decisionStatus != panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("inconsistency");
        issue.rule = QStringLiteral("R1 选中目标后三处状态显示必须一致");
        issue.action = action;
        issue.details = QStringLiteral("目标表=%1, 操作面板=%2, 决策面板=%3")
                            .arg(tableStatus, panelStatus, decisionStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R2：选中目标时按钮启用状态必须匹配当前状态
// confirm 启用 ⟺ Detected；start 启用 ⟺ Confirmed；complete 启用 ⟺ Disposing
void checkButtonEnableMatchesStatus(MainWindow &window, const QString &action,
                                    const QString &screenshotDir, int issueIdx,
                                    std::vector<Issue> &issues)
{
    const QString status = getPanelStatus(window);
    if (status == QStringLiteral("None")) {
        return; // 未选目标由 R3 覆盖
    }
    auto *confirm = findWidget<QPushButton>(window, "simulationConfirmButton");
    auto *start = findWidget<QPushButton>(window, "simulationStartButton");
    auto *complete = findWidget<QPushButton>(window, "simulationCompleteButton");

    QStringList mismatches;
    if (confirm != nullptr && confirm->isEnabled() != (status == QStringLiteral("Detected"))) {
        mismatches.append(QStringLiteral("确认按钮启用=%1 应为%2")
                              .arg(confirm->isEnabled())
                              .arg(status == QStringLiteral("Detected")));
    }
    if (start != nullptr && start->isEnabled() != (status == QStringLiteral("Confirmed"))) {
        mismatches.append(QStringLiteral("处置按钮启用=%1 应为%2")
                              .arg(start->isEnabled())
                              .arg(status == QStringLiteral("Confirmed")));
    }
    if (complete != nullptr && complete->isEnabled() != (status == QStringLiteral("Disposing"))) {
        mismatches.append(QStringLiteral("完成按钮启用=%1 应为%2")
                              .arg(complete->isEnabled())
                              .arg(status == QStringLiteral("Disposing")));
    }

    if (!mismatches.isEmpty()) {
        Issue issue;
        issue.type = QStringLiteral("inconsistency");
        issue.rule = QStringLiteral("R2 按钮启用状态必须匹配当前状态");
        issue.action = action;
        issue.details = QStringLiteral("状态=%1; %2").arg(status, mismatches.join(QStringLiteral("; ")));
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R3：未选目标时三个操作按钮必须全部禁用
void checkButtonsDisabledWhenNoSelection(MainWindow &window, const QString &action,
                                         const QString &screenshotDir, int issueIdx,
                                         std::vector<Issue> &issues)
{
    const QString panelTarget = getPanelTargetId(window);
    if (!panelTarget.contains(QStringLiteral("未选择")) && !panelTarget.isEmpty()) {
        return; // 已选目标，不适用
    }

    auto *confirm = findWidget<QPushButton>(window, "simulationConfirmButton");
    auto *start = findWidget<QPushButton>(window, "simulationStartButton");
    auto *complete = findWidget<QPushButton>(window, "simulationCompleteButton");

    QStringList enabledButtons;
    if (confirm != nullptr && confirm->isEnabled()) {
        enabledButtons.append(QStringLiteral("模拟确认"));
    }
    if (start != nullptr && start->isEnabled()) {
        enabledButtons.append(QStringLiteral("模拟处置"));
    }
    if (complete != nullptr && complete->isEnabled()) {
        enabledButtons.append(QStringLiteral("模拟完成"));
    }

    if (!enabledButtons.isEmpty()) {
        Issue issue;
        issue.type = QStringLiteral("disabled_violation");
        issue.rule = QStringLiteral("R3 未选目标时三按钮必须全禁用");
        issue.action = action;
        issue.details = QStringLiteral("未选目标但以下按钮可用：%1").arg(enabledButtons.join(QStringLiteral(", ")));
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R4：状态转移合法性。点击后状态只能保持或前进一步，不可跳级/倒退。
// 合法前进：Detected->Confirmed, Confirmed->Disposing, Disposing->Disposed
// 终态 Disposed 与无目标 None 只能保持。
void checkTransitionLegality(MainWindow &window, const Snapshot &pre, const QString &action,
                             const QString &screenshotDir, int issueIdx,
                             std::vector<Issue> &issues)
{
    const QString post = getPanelStatus(window);
    // 选前无目标（None/空）时，选后出现状态是"选择"而非"转移"，不适用本规则。
    if (pre.panelStatus.isEmpty() || pre.panelStatus == QStringLiteral("None")) {
        return;
    }
    if (pre.panelStatus == post) {
        return; // 保持不变，合法
    }

    // 判断是否前进一步
    const bool legalAdvance =
        (pre.panelStatus == QStringLiteral("Detected") && post == QStringLiteral("Confirmed"))
        || (pre.panelStatus == QStringLiteral("Confirmed") && post == QStringLiteral("Disposing"))
        || (pre.panelStatus == QStringLiteral("Disposing") && post == QStringLiteral("Disposed"));

    if (!legalAdvance) {
        Issue issue;
        issue.type = QStringLiteral("transition_violation");
        issue.rule = QStringLiteral("R4 状态转移必须合法（保持或前进一步）");
        issue.action = action;
        issue.details = QStringLiteral("点击前=%1, 点击后=%2（非法转移）").arg(pre.panelStatus, post);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R5：幂等。已选目标时再次点击目标行，状态不变且日志不增。
void checkTargetReselectIdempotent(MainWindow &window, const Snapshot &pre, int clickedRow,
                                   const QString &action, const QString &screenshotDir,
                                   int issueIdx, std::vector<Issue> &issues)
{
    if (clickedRow < 0) {
        return; // 本次动作不是点目标行
    }
    // 仅当选前已有目标选中时才适用（目标表无ID列，无法按ID精确匹配，
    // 演示场景仅一个目标，故以"选前已选"作为重选判据）。
    if (pre.panelTargetId.isEmpty() || pre.panelTargetId.contains(QStringLiteral("未选择"))) {
        return; // 选前未选目标，非重选场景
    }

    const QString postStatus = getPanelStatus(window);
    const int postLogCount = getLogLineCount(window);
    if (postStatus != pre.panelStatus || postLogCount != pre.logLineCount) {
        Issue issue;
        issue.type = QStringLiteral("inconsistency");
        issue.rule = QStringLiteral("R5 重复选同一目标必须幂等（状态与日志不变）");
        issue.action = action;
        issue.details = QStringLiteral("状态 前=%1 后=%2; 日志行 前=%3 后=%4")
                            .arg(pre.panelStatus, postStatus)
                            .arg(pre.logLineCount)
                            .arg(postLogCount);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R6：日志最后一条状态变更的新状态必须等于界面当前状态
void checkLogMatchesUi(MainWindow &window, const QString &action,
                       const QString &screenshotDir, int issueIdx,
                       std::vector<Issue> &issues)
{
    const QString logText = getLogText(window);
    if (logText.isEmpty() || logText.contains(QStringLiteral("暂无模拟操作记录"))) {
        return;
    }
    const QStringList lines = logText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return;
    }
    const QString lastLine = lines.last().trimmed();
    if (lastLine.contains(QStringLiteral("操作被拒绝"))) {
        return; // 拒绝记录，状态不变，此规则不适用
    }
    const int arrowIdx = lastLine.indexOf(QStringLiteral("->"));
    if (arrowIdx < 0) {
        return; // 非状态变更行，由 R8 检查格式
    }

    const QString logFinalStatusText = lastLine.mid(arrowIdx + 2).trimmed();
    const QString logFinalStatus = normalizeStatusText(logFinalStatusText);
    const QString panelStatus = getPanelStatus(window);

    if (logFinalStatus != panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("log_mismatch");
        issue.rule = QStringLiteral("R6 日志最后状态必须等于界面当前状态");
        issue.action = action;
        issue.details = QStringLiteral("日志最后状态=%1, 操作面板状态=%2")
                            .arg(logFinalStatusText, panelStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R7：日志增量每动作最多1行
void checkLogLineCountDelta(MainWindow &window, const Snapshot &pre, const QString &action,
                            const QString &screenshotDir, int issueIdx,
                            std::vector<Issue> &issues)
{
    const int post = getLogLineCount(window);
    const int delta = post - pre.logLineCount;
    if (delta < 0 || delta > 1) {
        Issue issue;
        issue.type = QStringLiteral("log_mismatch");
        issue.rule = QStringLiteral("R7 单次动作日志增量必须在0~1行");
        issue.action = action;
        issue.details = QStringLiteral("日志行 前=%1 后=%2 增量=%3").arg(pre.logLineCount).arg(post).arg(delta);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R8：日志格式一致。非占位行必须含 "->"（状态变更）或 "操作被拒绝" 或 "已选择目标"。
void checkLogFormatConsistency(MainWindow &window, const QString &action,
                               const QString &screenshotDir, int issueIdx,
                               std::vector<Issue> &issues)
{
    const QString logText = getLogText(window);
    if (logText.isEmpty() || logText.contains(QStringLiteral("暂无模拟操作记录"))) {
        return;
    }
    const QStringList lines = logText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList badLines;
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const bool ok = line.contains(QStringLiteral("->"))
                        || line.contains(QStringLiteral("操作被拒绝"))
                        || line.contains(QStringLiteral("已选择目标"));
        if (!ok) {
            badLines.append(line);
        }
    }

    if (!badLines.isEmpty()) {
        Issue issue;
        issue.type = QStringLiteral("log_mismatch");
        issue.rule = QStringLiteral("R8 日志非占位行必须含状态变更/拒绝/选择标记");
        issue.action = action;
        issue.details = QStringLiteral("格式异常行：%1").arg(badLines.join(QStringLiteral(" | ")));
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R9：选中目标后面板目标ID必须非空且非"未选择"
// 目标表不显示ID文本，无法做ID存在性匹配，退而检查选中后ID确实显示。
void checkPanelTargetIdExists(MainWindow &window, const QString &action,
                              const QString &screenshotDir, int issueIdx,
                              std::vector<Issue> &issues)
{
    const QString panelStatus = getPanelStatus(window);
    if (panelStatus == QStringLiteral("None")) {
        return; // 未选目标，不适用
    }
    const QString targetId = getPanelTargetId(window);
    if (targetId.isEmpty() || targetId.contains(QStringLiteral("未选择"))) {
        Issue issue;
        issue.type = QStringLiteral("id_mismatch");
        issue.rule = QStringLiteral("R9 选中目标后面板必须显示目标ID");
        issue.action = action;
        issue.details = QStringLiteral("已选目标但面板ID为空或未选择：%1").arg(targetId);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R10：点击目标行N后，面板目标ID必须从"未选择"变为具体值
void checkTargetRowClickConsistency(MainWindow &window, int clickedRow, const QString &action,
                                    const QString &screenshotDir, int issueIdx,
                                    std::vector<Issue> &issues)
{
    if (clickedRow < 0) {
        return; // 本次动作不是点目标行
    }
    const QString panelId = getPanelTargetId(window);
    if (panelId.isEmpty() || panelId.contains(QStringLiteral("未选择"))) {
        Issue issue;
        issue.type = QStringLiteral("id_mismatch");
        issue.rule = QStringLiteral("R10 点击目标行后面板必须显示该目标ID");
        issue.action = action;
        issue.details = QStringLiteral("点击第%1行后面板ID仍为空或未选择：%2").arg(clickedRow).arg(panelId);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R11：切换标签页后，QTabWidget 的 currentIndex 必须等于目标索引
void checkTabSwitchConsistency(MainWindow &window, int expectedTab,
                                const QString &action, const QString &screenshotDir,
                                int issueIdx, std::vector<Issue> &issues)
{
    if (expectedTab < 0) {
        return;
    }
    auto *tabs = findTabWidget(window);
    if (tabs == nullptr) {
        return;
    }
    if (tabs->currentIndex() != expectedTab) {
        Issue issue;
        issue.type = QStringLiteral("inconsistency");
        issue.rule = QStringLiteral("R11 切换标签页后当前索引必须匹配");
        issue.action = action;
        issue.details = QStringLiteral("期望=%1 实际=%2").arg(expectedTab).arg(tabs->currentIndex());
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R12：刷新按钮不应丢失已选目标（panelTargetId 和 panelStatus 必须保持）
void checkRefreshPreservesSelection(MainWindow &window, const Snapshot &pre,
                                     const QString &action, const QString &screenshotDir,
                                     int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("刷新"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelTargetId != post.panelTargetId || pre.panelStatus != post.panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("state_loss");
        issue.rule = QStringLiteral("R12 刷新后不应丢失已选目标");
        issue.action = action;
        issue.details = QStringLiteral("刷新前 target=%1 status=%2 | 刷新后 target=%3 status=%4")
                            .arg(pre.panelTargetId, pre.panelStatus, post.panelTargetId, post.panelStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R13：相机操作不应改变模拟状态机（panelStatus 和日志行数不变）
void checkCameraDoesNotCorruptState(MainWindow &window, const Snapshot &pre,
                                     const QString &action, const QString &screenshotDir,
                                     int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("俯")) && !action.contains(QStringLiteral("侧"))
        && !action.contains(QStringLiteral("3D")) && !action.contains(QStringLiteral("相机复位"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelStatus != post.panelStatus || pre.logLineCount != post.logLineCount) {
        Issue issue;
        issue.type = QStringLiteral("side_effect");
        issue.rule = QStringLiteral("R13 相机操作不应改变模拟状态或日志");
        issue.action = action;
        issue.details = QStringLiteral("操作前 status=%1 log=%2 | 操作后 status=%3 log=%4")
                            .arg(pre.panelStatus).arg(pre.logLineCount)
                            .arg(post.panelStatus).arg(post.logLineCount);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R14：任何动作后主窗口仍可见（菜单"退出"等动作不应意外关闭窗口）
void checkWindowStillVisible(MainWindow &window, const QString &action,
                              const QString &screenshotDir, int issueIdx,
                              std::vector<Issue> &issues)
{
    if (window.isVisible()) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("window_closed");
    issue.rule = QStringLiteral("R14 动作后主窗口仍可见");
    issue.action = action;
    issue.details = QStringLiteral("执行动作后主窗口不可见（疑似意外关闭）");
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R15：视图切换（菜单 checkable 切换面板显示）后恢复，状态不丢失。
// ViewToggle 动作会在 toggle 后再 toggle 回来恢复原状态；本规则验证恢复后状态保持。
void checkViewTogglePreservesState(MainWindow &window, const Snapshot &pre,
                                    const QString &action, const QString &screenshotDir,
                                    int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("视图切换"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelStatus != post.panelStatus || pre.panelTargetId != post.panelTargetId
        || pre.logLineCount != post.logLineCount) {
        Issue issue;
        issue.type = QStringLiteral("state_loss");
        issue.rule = QStringLiteral("R15 视图切换恢复后状态应保持");
        issue.action = action;
        issue.details = QStringLiteral("操作前 status=%1 target=%2 log=%3 | 恢复后 status=%4 target=%5 log=%6")
                            .arg(pre.panelStatus, pre.panelTargetId).arg(pre.logLineCount)
                            .arg(post.panelStatus, post.panelTargetId).arg(post.logLineCount);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R16：搜索框输入不应改变选中状态（panelTargetId 和 panelStatus 保持）
void checkSearchPreservesSelection(MainWindow &window, const Snapshot &pre,
                                    const QString &action, const QString &screenshotDir,
                                    int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("搜索框"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelTargetId != post.panelTargetId || pre.panelStatus != post.panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("state_loss");
        issue.rule = QStringLiteral("R16 搜索框输入不应改变选中状态");
        issue.action = action;
        issue.details = QStringLiteral("操作前 target=%1 status=%2 | 操作后 target=%3 status=%4")
                            .arg(pre.panelTargetId, pre.panelStatus, post.panelTargetId, post.panelStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R17：任务/设备表行点击不应改变目标选中状态（panelTargetId 和 panelStatus 保持）
void checkTableRowClickPreservesSelection(MainWindow &window, const Snapshot &pre,
                                           const QString &action, const QString &screenshotDir,
                                           int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("任务表")) && !action.contains(QStringLiteral("设备表"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelTargetId != post.panelTargetId || pre.panelStatus != post.panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("state_loss");
        issue.rule = QStringLiteral("R17 任务/设备表行点击不应改变目标选中状态");
        issue.action = action;
        issue.details = QStringLiteral("操作前 target=%1 status=%2 | 操作后 target=%3 status=%4")
                            .arg(pre.panelTargetId, pre.panelStatus, post.panelTargetId, post.panelStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R18：状态子标签页点击不应改变目标选中状态
void checkStatusTabClickPreservesSelection(MainWindow &window, const Snapshot &pre,
                                            const QString &action, const QString &screenshotDir,
                                            int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("状态标签"))) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    if (pre.panelTargetId != post.panelTargetId || pre.panelStatus != post.panelStatus) {
        Issue issue;
        issue.type = QStringLiteral("state_loss");
        issue.rule = QStringLiteral("R18 状态子标签页点击不应改变目标选中状态");
        issue.action = action;
        issue.details = QStringLiteral("操作前 target=%1 status=%2 | 操作后 target=%3 status=%4")
                            .arg(pre.panelTargetId, pre.panelStatus, post.panelTargetId, post.panelStatus);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// R19：键盘导航（Tab/Enter/Esc/方向键）不应丢失选中（panelTargetId 不应从有值变为空）。
// 注意：Enter 可能触发聚焦按钮导致状态机变更，这是合法的（由 R4 覆盖），
// 故 R19 只检查选中丢失，不检查状态变更。
void checkKeyboardNavPreservesSelection(MainWindow &window, const Snapshot &pre,
                                         const QString &action, const QString &screenshotDir,
                                         int issueIdx, std::vector<Issue> &issues)
{
    if (!action.contains(QStringLiteral("键盘"))) {
        return;
    }
    // 仅当选前已有选中、操作后选中丢失时才报告
    const bool hadSelection = !pre.panelTargetId.isEmpty()
                              && !pre.panelTargetId.contains(QStringLiteral("未选择"));
    if (!hadSelection) {
        return;
    }
    const Snapshot post = captureSnapshot(window);
    const bool lostSelection = post.panelTargetId.isEmpty()
                               || post.panelTargetId.contains(QStringLiteral("未选择"));
    if (!lostSelection) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("state_loss");
    issue.rule = QStringLiteral("R19 键盘导航不应丢失选中");
    issue.action = action;
    issue.details = QStringLiteral("操作前 target=%1 | 操作后 target=%2（选中丢失）")
                        .arg(pre.panelTargetId, post.panelTargetId);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// ===== 动作执行 =====

enum class ActionKind {
    TargetRow,
    Confirm,
    Start,
    Complete,
    CameraTop,
    CameraSide,
    Camera3D,
    CameraReset,
    TabSwitch,
    Refresh,
    MenuAction,
    ViewToggle,
    SearchInput,
    NavButton,
    MissionRow,
    DeviceRow,
    StatusTab,
    KeyTab,
    KeyEnter,
    KeyEscape,
    KeyArrow,
    MenuHoverTiming, // 测量菜单关闭延迟（用户原问题）
    SearchRobust,    // 搜索框注入对抗性输入（超长/特殊字符/emoji/RTL），验证不崩溃不乱码
};

struct ActionResult {
    QString description;
    bool executed;
    ActionKind kind = ActionKind::TargetRow;
    int clickedRow = -1; // 仅 TargetRow 有效
    int tabTarget = -1;  // 仅 TabSwitch 有效
    int navTarget = -1;  // 仅 NavButton 有效
    qint64 timingMs = -1; // 动作耗时（click+processEvents），-1 表示未测量
    QString dialogTitle;  // 弹出对话框的标题（exec 期间抓取），空表示无对话框弹出
    bool dialogShown = false; // 是否弹出了模态对话框
    bool enabledBefore = false; // 点击前按钮是否启用（仅 Confirm/Start/Complete 有效，供 R30 检查）
    quintptr focusBeforeAddr = 0; // Tab 前焦点控件地址（仅 KeyTab 有效，供 R31 检查）
    quintptr focusAfterAddr = 0;  // Tab 后焦点控件地址（仅 KeyTab 有效，供 R31 检查）
};

// R20：菜单关闭延迟应 <=300ms（用户原问题：下拉菜单过一阵才消失）。
// 仅对 MenuHoverTiming 动作生效（timingMs >= 0）。
void checkMenuCloseTiming(qint64 timingMs, ActionKind kind, MainWindow &window,
                          const QString &action, const QString &screenshotDir,
                          int issueIdx, std::vector<Issue> &issues)
{
    const qint64 threshold = 300;
    if (kind != ActionKind::MenuHoverTiming || timingMs < 0 || timingMs <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R20 菜单关闭延迟应 <=300ms");
    issue.action = action;
    issue.details = QStringLiteral("关闭延迟 %1ms 超过阈值 %2ms").arg(timingMs).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R21：状态机按钮点击响应应 <=200ms（confirm/start/complete）。
void checkButtonResponseTiming(qint64 timingMs, ActionKind kind, MainWindow &window,
                               const QString &action, const QString &screenshotDir,
                               int issueIdx, std::vector<Issue> &issues)
{
    const qint64 threshold = 200;
    const bool isStateButton = (kind == ActionKind::Confirm
                                || kind == ActionKind::Start
                                || kind == ActionKind::Complete);
    if (!isStateButton || timingMs < 0 || timingMs <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R21 状态机按钮响应应 <=200ms");
    issue.action = action;
    issue.details = QStringLiteral("响应耗时 %1ms 超过阈值 %2ms").arg(timingMs).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R22：标签页切换响应应 <=200ms。
void checkTabSwitchTiming(qint64 timingMs, ActionKind kind, MainWindow &window,
                          const QString &action, const QString &screenshotDir,
                          int issueIdx, std::vector<Issue> &issues)
{
    const qint64 threshold = 200;
    if (kind != ActionKind::TabSwitch || timingMs < 0 || timingMs <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R22 标签页切换响应应 <=200ms");
    issue.action = action;
    issue.details = QStringLiteral("切换耗时 %1ms 超过阈值 %2ms").arg(timingMs).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R23：搜索框输入过滤响应应 <=200ms（onSearchTextChanged 同步遍历全部行列做 contains + setRowHidden）。
void checkSearchFilterTiming(qint64 timingMs, ActionKind kind, MainWindow &window,
                             const QString &action, const QString &screenshotDir,
                             int issueIdx, std::vector<Issue> &issues)
{
    const qint64 threshold = 200;
    if (kind != ActionKind::SearchInput || timingMs < 0 || timingMs <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R23 搜索过滤响应应 <=200ms");
    issue.action = action;
    issue.details = QStringLiteral("过滤耗时 %1ms 超过阈值 %2ms").arg(timingMs).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R24：模态对话框弹出时标题应非空（捕获 i18n/tr() 缺失或编码异常）。
void checkDialogTitleNotEmpty(ActionKind kind, bool dialogShown, const QString &dialogTitle,
                              MainWindow &window, const QString &action,
                              const QString &screenshotDir, int issueIdx,
                              std::vector<Issue> &issues)
{
    if (kind != ActionKind::MenuAction || !dialogShown) {
        return;
    }
    if (!dialogTitle.isEmpty()) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("dialog");
    issue.rule = QStringLiteral("R24 对话框标题应非空");
    issue.action = action;
    issue.details = QStringLiteral("弹出模态对话框但标题为空（疑似 tr() 缺失或编码异常）");
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R25：对话框关闭后不应残留模态状态（activeModalWidget 应为 null）。
void checkNoResidualModalAfterClose(ActionKind kind, bool dialogShown, MainWindow &window,
                                    const QString &action, const QString &screenshotDir,
                                    int issueIdx, std::vector<Issue> &issues)
{
    if (kind != ActionKind::MenuAction || !dialogShown) {
        return;
    }
    QWidget *modal = QApplication::activeModalWidget();
    if (modal == nullptr) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("dialog");
    issue.rule = QStringLiteral("R25 对话框关闭后不应残留模态状态");
    issue.action = action;
    issue.details = QStringLiteral("对话框已关闭但 activeModalWidget 仍非空（疑似关闭失败或挂起）");
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R26：超长文本过滤响应应 <=500ms（对抗性输入不应导致主线程长时间卡顿）。
// 阈值比 R23 宽（500ms vs 200ms），因为 SearchRobust 注入的超长文本会放大 O(rows×cols) 遍历成本。
void checkSearchRobustTiming(qint64 timingMs, ActionKind kind, MainWindow &window,
                             const QString &action, const QString &screenshotDir,
                             int issueIdx, std::vector<Issue> &issues)
{
    const qint64 threshold = 500;
    if (kind != ActionKind::SearchRobust || timingMs < 0 || timingMs <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R26 超长文本过滤响应应 <=500ms");
    issue.action = action;
    issue.details = QStringLiteral("对抗性输入过滤耗时 %1ms 超过阈值 %2ms").arg(timingMs).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R27：标签页切换后当前页 currentWidget 应非空且可见（捕获页面损坏/空页/隐藏页 bug）。
// 仅对 TabSwitch 动作触发，避免对其他动作误报。
void checkTabPageVisibleAfterSwitch(ActionKind kind, MainWindow &window, int expectedTab,
                                    const QString &action, const QString &screenshotDir,
                                    int issueIdx, std::vector<Issue> &issues)
{
    if (kind != ActionKind::TabSwitch || expectedTab < 0) {
        return;
    }
    auto *tabs = findTabWidget(window);
    if (tabs == nullptr) {
        return;
    }
    QWidget *page = tabs->currentWidget();
    if (page == nullptr || !page->isVisible()) {
        Issue issue;
        issue.type = QStringLiteral("visual");
        issue.rule = QStringLiteral("R27 标签页切换后当前页应非空且可见");
        issue.action = action;
        issue.details = QStringLiteral("expectedTab=%1 page=%2 visible=%3")
                            .arg(expectedTab)
                            .arg(page == nullptr ? QStringLiteral("null") : QStringLiteral("非空"))
                            .arg(page == nullptr ? 0 : (page->isVisible() ? 1 : 0));
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// 前置声明：actionKindToString 定义在 pickAndExecuteAction 附近，R28 在其之前使用
QString actionKindToString(ActionKind kind);

// R28：非状态机动作不应改变 panelStatus（当已选目标时）。
// 可合法改变状态的动作：Confirm/Start/Complete/TargetRow（选择目标）/KeyEnter（可能触发聚焦按钮）。
// 其他动作（相机/刷新/标签/搜索/导航/键盘 Tab/Esc/箭头/菜单/视图/任务表/设备表/状态子标签/菜单悬停/对抗性搜索）
// 若改变了 panelStatus，说明业务侧有副作用 bug（如刷新触发状态重置、搜索触发误迁移）。
void checkNonStateActionPreservesStatus(ActionKind kind, MainWindow &window, const Snapshot &pre,
                                        const QString &action, const QString &screenshotDir,
                                        int issueIdx, std::vector<Issue> &issues)
{
    // 选前无目标时，状态本就是 None，不适用本规则（由 R4 覆盖选择行为）
    if (pre.panelStatus.isEmpty() || pre.panelStatus == QStringLiteral("None")) {
        return;
    }
    // 可合法改变状态的动作
    switch (kind) {
    case ActionKind::Confirm:
    case ActionKind::Start:
    case ActionKind::Complete:
    case ActionKind::TargetRow:
    case ActionKind::KeyEnter:
        return;
    default:
        break;
    }
    const QString post = getPanelStatus(window);
    if (post == pre.panelStatus) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("business_flow");
    issue.rule = QStringLiteral("R28 非状态机动作不应改变状态");
    issue.action = action;
    issue.details = QStringLiteral("动作=%1 前=%2 后=%3（非状态机动作意外改变了状态）")
                        .arg(actionKindToString(kind), pre.panelStatus, post);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R29：会话级 widget 子对象数不应显著增长（孤儿对话框/定时器累积未清理）。
// 阈值 +30：允许正常波动（菜单/对话框短暂创建），超过则可能存在泄漏。
// 在主循环结束后调用一次，非 per-action 检查。
void checkSessionWidgetLeak(MainWindow &window, int initialChildCount,
                            const QString &screenshotDir, int issueIdx,
                            std::vector<Issue> &issues)
{
    const int finalChildCount = window.children().size();
    const int growth = finalChildCount - initialChildCount;
    const int threshold = 30;
    if (growth <= threshold) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("stability");
    issue.rule = QStringLiteral("R29 会话级 widget 子对象数不应显著增长");
    issue.action = QStringLiteral("会话结束");
    issue.details = QStringLiteral("初始=%1 最终=%2 增长=%3 超过阈值%4（可能存在孤儿对象累积）")
                        .arg(initialChildCount).arg(finalChildCount).arg(growth).arg(threshold);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R32：会话级响应时间退化趋势检测（A5 性能守护）。
// 比较前 20% 与后 20% 动作的平均耗时，若后 20% 显著慢于前 20%
// （比值 >2 且绝对差 >50ms），说明性能在会话过程中退化（内存压力/状态累积/缓存膨胀）。
// 仅统计 timingMs >= 0 的动作（有计时器的动作：clickButton/tabSwitch/searchInput/
// searchRobust/menuHoverTiming），在主循环结束后调用一次。
// 双阈值（比值+绝对差）避免小绝对差的噪声误报（如 5ms->11ms 是 2.2x 但仅 6ms 差）。
void checkSessionResponseTimeDegradation(const QVector<qint64> &timings,
                                         MainWindow &window,
                                         const QString &screenshotDir, int issueIdx,
                                         std::vector<Issue> &issues)
{
    const int n = timings.size();
    if (n < 10) {
        return; // 样本不足，无法可靠比较
    }
    const int bucketSize = n / 5; // 20%
    if (bucketSize < 3) {
        return; // 每桶样本不足
    }
    qint64 firstSum = 0;
    for (int i = 0; i < bucketSize; ++i) {
        firstSum += timings[i];
    }
    const double firstAvg = static_cast<double>(firstSum) / bucketSize;
    qint64 lastSum = 0;
    for (int i = n - bucketSize; i < n; ++i) {
        lastSum += timings[i];
    }
    const double lastAvg = static_cast<double>(lastSum) / bucketSize;
    const double ratio = firstAvg > 0 ? lastAvg / firstAvg : 0;
    const double absDiff = lastAvg - firstAvg;
    if (ratio <= 2.0 || absDiff <= 50.0) {
        return;
    }
    Issue issue;
    issue.type = QStringLiteral("performance");
    issue.rule = QStringLiteral("R32 会话级响应时间退化趋势");
    issue.action = QStringLiteral("会话结束");
    issue.details = QStringLiteral("前20%%平均=%1ms 后20%%平均=%2ms 比值=%3x 绝对差=%4ms（性能在会话过程中显著退化）")
                        .arg(firstAvg, 0, 'f', 1).arg(lastAvg, 0, 'f', 1)
                        .arg(ratio, 0, 'f', 2).arg(absDiff, 0, 'f', 1);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R30：启用的状态机按钮点击后必须触发状态前进（捕获 enabled 但 no-op 的按钮 bug）。
// R4 只检查"发生了的迁移是否合法"，R30 补齐"启用的按钮是否真的触发了迁移"。
// 仅对 Confirm/Start/Complete 生效，依赖 ActionResult.enabledBefore 与 pre.panelStatus。
void checkEnabledButtonCausesTransition(ActionKind kind, bool enabledBefore,
                                        MainWindow &window, const Snapshot &pre,
                                        const QString &action, const QString &screenshotDir,
                                        int issueIdx, std::vector<Issue> &issues)
{
    if (!enabledBefore) {
        return; // 禁用按钮无反应是正常的（由 R2/R3 覆盖）
    }
    QString expectedPost;
    switch (kind) {
    case ActionKind::Confirm:
        if (pre.panelStatus != QStringLiteral("Detected")) {
            return; // 状态不匹配说明 R2 已报警，不重复
        }
        expectedPost = QStringLiteral("Confirmed");
        break;
    case ActionKind::Start:
        if (pre.panelStatus != QStringLiteral("Confirmed")) {
            return;
        }
        expectedPost = QStringLiteral("Disposing");
        break;
    case ActionKind::Complete:
        if (pre.panelStatus != QStringLiteral("Disposing")) {
            return;
        }
        expectedPost = QStringLiteral("Disposed");
        break;
    default:
        return; // 非状态机按钮不适用
    }
    const QString post = getPanelStatus(window);
    if (post == expectedPost) {
        return; // 正确前进
    }
    Issue issue;
    issue.type = QStringLiteral("business_flow");
    issue.rule = QStringLiteral("R30 启用的状态机按钮必须触发状态前进");
    issue.action = action;
    issue.details = QStringLiteral("按钮已启用 前=%1 期望后=%2 实际后=%3（启用的按钮点击后无反应）")
                        .arg(pre.panelStatus, expectedPost, post);
    issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
    captureScreenshot(window, screenshotDir, issue.screenshot);
    issues.push_back(issue);
}

// R31：Tab 键后焦点应非空且发生变化（捕获焦点陷阱/死循环/焦点丢失）。
// 仅对 KeyTab 生效，依赖 ActionResult.focusBeforeAddr/focusAfterAddr。
// offscreen 下 Tab 仍走 Qt 默认焦点链，焦点不变通常意味着焦点陷阱或
// nextPrevChild 被重写为 no-op（业务侧常见 bug）。
void checkTabFocusMoved(ActionKind kind, quintptr focusBeforeAddr, quintptr focusAfterAddr,
                        MainWindow &window, const QString &action,
                        const QString &screenshotDir, int issueIdx,
                        std::vector<Issue> &issues)
{
    if (kind != ActionKind::KeyTab) {
        return;
    }
    if (focusAfterAddr == 0) {
        // Tab 后焦点丢失
        Issue issue;
        issue.type = QStringLiteral("keyboard");
        issue.rule = QStringLiteral("R31 Tab 键后焦点不应丢失");
        issue.action = action;
        issue.details = QStringLiteral("Tab 后 focusWidget 为空（焦点链断裂，可能丢失到不可见控件或被清空）");
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
        return;
    }
    if (focusBeforeAddr != 0 && focusAfterAddr == focusBeforeAddr) {
        // 焦点未变化。Tab 在以下控件上不前进焦点是合法的：
        //   - 文本输入控件（QLineEdit/QTextEdit/QComboBox/QAbstractSpinBox 等，Tab 用于输入）
        //   - 项视图（QAbstractItemView 子类，Tab 用于单元格/项导航，Qt 默认行为）
        // 仅当焦点控件非"Tab 消费型"时才报警。
        auto *w = QApplication::focusWidget();
        const QString cls = w != nullptr ? w->metaObject()->className() : QString();
        const QString name = w != nullptr && !w->objectName().isEmpty() ? w->objectName() : QStringLiteral("(无名)");
        const bool isTabConsumer =
            (w != nullptr)
            && (w->inherits("QLineEdit") || w->inherits("QTextEdit")
                || w->inherits("QPlainTextEdit") || w->inherits("QComboBox")
                || w->inherits("QAbstractSpinBox") || w->inherits("QAbstractItemView"));
        if (isTabConsumer) {
            return; // 合法：Tab 被输入控件或项视图消费
        }
        Issue issue;
        issue.type = QStringLiteral("keyboard");
        issue.rule = QStringLiteral("R31 Tab 键后焦点应变化");
        issue.action = action;
        issue.details = QStringLiteral("Tab 前后焦点控件相同（焦点陷阱或 nextPrevChild 被 no-op，焦点链死循环）控件=%1:%2")
                            .arg(cls, name);
        issue.screenshot = QStringLiteral("issue_%1.png").arg(issueIdx);
        captureScreenshot(window, screenshotDir, issue.screenshot);
        issues.push_back(issue);
    }
}

// 点击目标表第 row 行（选中目标）
ActionResult clickTargetRow(MainWindow &window, int row)
{
    auto *table = findWidget<QTableWidget>(window, "targetTable");
    if (table == nullptr || row >= table->rowCount()) {
        return {QStringLiteral("无效目标行"), false};
    }
    QTableWidgetItem *item = table->item(row, 0);
    if (item == nullptr) {
        return {QStringLiteral("目标单元格无效"), false};
    }
    table->scrollToItem(item);
    const QRect rect = table->visualItemRect(item);
    if (!rect.isValid()) {
        return {QStringLiteral("目标单元格不可见"), false};
    }
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
    return {QStringLiteral("点击目标表第%1行").arg(row), true, ActionKind::TargetRow, row};
}

// 点击指定按钮
ActionResult clickButton(MainWindow &window, const char *objectName,
                         const QString &displayName, ActionKind kind)
{
    auto *btn = findWidget<QPushButton>(window, objectName);
    if (btn == nullptr) {
        return {QStringLiteral("找不到按钮 %1").arg(displayName), false, kind};
    }
    // 对禁用按钮也执行点击--mouseClick 不会触发 clicked 信号，
    // 随后的规则检查会验证状态确实没变（禁用按钮不应有反应）。
    const bool enabledBefore = btn->isEnabled();
    QElapsedTimer t;
    t.start();
    QTest::mouseClick(btn, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    ActionResult r{QStringLiteral("点击 %1 按钮").arg(displayName), true, kind};
    r.timingMs = t.elapsed();
    r.enabledBefore = enabledBefore;
    return r;
}

// 点击无 objectName 的按钮（相机/刷新），按文本查找
ActionResult clickButtonByText(MainWindow &window, const QString &text,
                                const QString &displayName, ActionKind kind)
{
    auto *btn = findButtonByText(window, text);
    if (btn == nullptr) {
        return {QStringLiteral("找不到按钮 %1").arg(displayName), false, kind};
    }
    QElapsedTimer t;
    t.start();
    QTest::mouseClick(btn, Qt::LeftButton);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    ActionResult r{QStringLiteral("点击 %1 按钮").arg(displayName), true, kind};
    r.timingMs = t.elapsed();
    return r;
}

// 切换到指定标签页（0=目标, 1=任务, 2=设备）
ActionResult clickTab(MainWindow &window, int tabIndex)
{
    auto *tabs = findTabWidget(window);
    if (tabs == nullptr || tabIndex >= tabs->count()) {
        return {QStringLiteral("找不到标签页或索引越界"), false, ActionKind::TabSwitch, -1, tabIndex};
    }
    QElapsedTimer t;
    t.start();
    tabs->setCurrentIndex(tabIndex);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    ActionResult r{QStringLiteral("切换到标签页 %1").arg(tabs->tabText(tabIndex)), true, ActionKind::TabSwitch, -1, tabIndex};
    r.timingMs = t.elapsed();
    return r;
}

// 触发菜单 action。部分菜单（文件菜单）会弹 QMessageBox，预设定时器自动关闭。
ActionResult triggerMenuAction(MainWindow &window, const MenuActionInfo &info)
{
    if (info.action == nullptr) {
        return {QStringLiteral("菜单 action 无效"), false, ActionKind::MenuAction};
    }
    ActionResult r{QStringLiteral("触发菜单 %1 > %2").arg(info.menuName, info.actionText),
                   true, ActionKind::MenuAction};
    scheduleModalDialogAutoClose(&r.dialogTitle, &r.dialogShown);
    info.action->trigger();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    // 二次确认关闭残留对话框（exec 可能在 processEvents 后才弹出）
    QWidget *modal = QApplication::activeModalWidget();
    if (modal != nullptr) {
        if (!r.dialogShown) {
            r.dialogShown = true;
            r.dialogTitle = modal->windowTitle();
        }
        modal->close();
    }
    return r;
}

// 切换视图菜单的 checkable action，再切换回来恢复原状态。
// 验证隐藏/显示面板后状态不被破坏。
ActionResult toggleViewAction(MainWindow &window, QAction *action, int index)
{
    if (action == nullptr) {
        return {QStringLiteral("视图 action 无效"), false, ActionKind::ViewToggle};
    }
    action->trigger();
    QTest::qWait(50);
    action->trigger();
    QTest::qWait(50);
    return {QStringLiteral("视图切换 %1 (恢复)").arg(action->text()), true, ActionKind::ViewToggle, -1, -1, index};
}

// 在搜索框输入文本（测试筛选不崩溃）
ActionResult inputSearchText(MainWindow &window, const QString &text)
{
    auto *edit = findSearchEdit(window);
    if (edit == nullptr) {
        return {QStringLiteral("找不到搜索框"), false, ActionKind::SearchInput};
    }
    // 测量 setText 触发 textChanged -> onSearchTextChanged 同步过滤的耗时
    QElapsedTimer t;
    t.start();
    edit->setText(text);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    ActionResult r{QStringLiteral("搜索框输入 %1").arg(text), true, ActionKind::SearchInput};
    r.timingMs = t.elapsed();
    // 恢复：清空搜索框（不测量，仅恢复界面状态）
    edit->clear();
    QTest::qWait(30);
    return r;
}

// 搜索框注入对抗性输入（超长文本/CRLF/正则字符/emoji/RTL），验证不崩溃不乱码不卡顿。
// description 截断到 30 字符避免超长文本污染 action_log 与 STATUS.md。
ActionResult inputSearchRobust(MainWindow &window, const QString &label, const QString &text)
{
    auto *edit = findSearchEdit(window);
    if (edit == nullptr) {
        return {QStringLiteral("找不到搜索框"), false, ActionKind::SearchRobust};
    }
    QElapsedTimer t;
    t.start();
    edit->setText(text);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QString desc = label;
    if (desc.size() > 30) {
        desc = desc.left(30) + QStringLiteral("...");
    }
    ActionResult r{QStringLiteral("搜索框注入 %1").arg(desc), true, ActionKind::SearchRobust};
    r.timingMs = t.elapsed();
    edit->clear();
    QTest::qWait(30);
    return r;
}

// 点击导航按钮（6 个：态势/探测/决策/设备/统计/配置）
ActionResult clickNavButton(MainWindow &window, int navIndex)
{
    const auto buttons = findNavButtons(window);
    if (navIndex < 0 || navIndex >= buttons.size()) {
        return {QStringLiteral("导航按钮索引越界"), false, ActionKind::NavButton, -1, -1, navIndex};
    }
    QTest::mouseClick(buttons[navIndex], Qt::LeftButton);
    return {QStringLiteral("点击导航按钮 #%1").arg(navIndex), true, ActionKind::NavButton, -1, -1, navIndex};
}

// 点击任务表第 row 行（missionSelected 信号当前无消费者，验证点击不崩溃/不破坏选中）
ActionResult clickMissionRow(MainWindow &window, int row)
{
    auto *table = findMissionTable(window);
    if (table == nullptr || row >= table->rowCount()) {
        return {QStringLiteral("任务表无效或行越界"), false, ActionKind::MissionRow};
    }
    QTableWidgetItem *item = table->item(row, 0);
    if (item == nullptr) {
        return {QStringLiteral("任务表单元格无效"), false, ActionKind::MissionRow};
    }
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                      table->visualItemRect(item).center());
    return {QStringLiteral("点击任务表第%1行").arg(row), true, ActionKind::MissionRow};
}

// 点击设备表第 row 行（deviceSelected 信号当前无消费者，验证点击不崩溃/不破坏选中）
ActionResult clickDeviceRow(MainWindow &window, int row)
{
    auto *table = findDeviceTable(window);
    if (table == nullptr || row >= table->rowCount()) {
        return {QStringLiteral("设备表无效或行越界"), false, ActionKind::DeviceRow};
    }
    QTableWidgetItem *item = table->item(row, 0);
    if (item == nullptr) {
        return {QStringLiteral("设备表单元格无效"), false, ActionKind::DeviceRow};
    }
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                      table->visualItemRect(item).center());
    return {QStringLiteral("点击设备表第%1行").arg(row), true, ActionKind::DeviceRow};
}

// 点击状态子标签页按钮（待处置/处置中/已完成任务计数）
ActionResult clickStatusTab(MainWindow &window, int tabIndex)
{
    const auto buttons = findStatusTabButtons(window);
    if (tabIndex < 0 || tabIndex >= buttons.size()) {
        return {QStringLiteral("状态标签页索引越界"), false, ActionKind::StatusTab};
    }
    QTest::mouseClick(buttons[tabIndex], Qt::LeftButton);
    return {QStringLiteral("点击状态标签 #%1").arg(tabIndex), true, ActionKind::StatusTab};
}

// 键盘导航：对当前焦点控件发送 Tab 键（焦点切换，Qt 默认行为）
ActionResult sendKeyTab(MainWindow &window)
{
    QWidget *focus = QApplication::focusWidget();
    if (focus == nullptr) {
        window.setFocus();
        focus = QApplication::focusWidget();
        if (focus == nullptr) {
            return {QStringLiteral("无焦点控件，跳过 Tab"), false, ActionKind::KeyTab};
        }
    }
    const quintptr beforeAddr = reinterpret_cast<quintptr>(focus);
    QTest::keyClick(focus, Qt::Key_Tab);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QWidget *after = QApplication::focusWidget();
    const quintptr afterAddr = after != nullptr ? reinterpret_cast<quintptr>(after) : 0;
    ActionResult r{QStringLiteral("键盘 Tab（焦点切换）"), true, ActionKind::KeyTab};
    r.focusBeforeAddr = beforeAddr;
    r.focusAfterAddr = afterAddr;
    return r;
}

// 键盘导航：对当前焦点控件发送 Enter 键（可能触发按钮点击或行激活）
ActionResult sendKeyEnter(MainWindow &window)
{
    QWidget *focus = QApplication::focusWidget();
    if (focus == nullptr) {
        window.setFocus();
        focus = QApplication::focusWidget();
        if (focus == nullptr) {
            return {QStringLiteral("无焦点控件，跳过 Enter"), false, ActionKind::KeyEnter};
        }
    }
    QTest::keyClick(focus, Qt::Key_Return);
    return {QStringLiteral("键盘 Enter"), true, ActionKind::KeyEnter};
}

// 键盘导航：对当前焦点控件发送 Esc 键（关闭模态或清空选中）
ActionResult sendKeyEscape(MainWindow &window)
{
    QWidget *focus = QApplication::focusWidget();
    if (focus == nullptr) {
        window.setFocus();
        focus = QApplication::focusWidget();
        if (focus == nullptr) {
            return {QStringLiteral("无焦点控件，跳过 Esc"), false, ActionKind::KeyEscape};
        }
    }
    QTest::keyClick(focus, Qt::Key_Escape);
    return {QStringLiteral("键盘 Esc"), true, ActionKind::KeyEscape};
}

// 键盘导航：对当前焦点控件发送方向键（上下左右随机选一个）
ActionResult sendKeyArrow(MainWindow &window, QRandomGenerator &rng)
{
    QWidget *focus = QApplication::focusWidget();
    if (focus == nullptr) {
        window.setFocus();
        focus = QApplication::focusWidget();
        if (focus == nullptr) {
            return {QStringLiteral("无焦点控件，跳过方向键"), false, ActionKind::KeyArrow};
        }
    }
    const Qt::Key arrows[4] = {Qt::Key_Up, Qt::Key_Down, Qt::Key_Left, Qt::Key_Right};
    const Qt::Key key = arrows[rng.bounded(4)];
    QTest::keyClick(focus, key);
    return {QStringLiteral("键盘方向键 %1").arg(static_cast<int>(key)), true, ActionKind::KeyArrow};
}

// 菜单关闭延迟测量（用户原问题：点击菜单后移开，下拉菜单过一阵才消失）。
// offscreen 下鼠标移开不可复现，改用 Esc 关闭测量耗时；结果存入 timingMs。
ActionResult menuHoverTimingAction(MainWindow &window, QRandomGenerator &rng)
{
    auto *bar = window.menuBar();
    if (bar == nullptr) {
        return {QStringLiteral("菜单栏无效"), false, ActionKind::MenuHoverTiming};
    }

    QVector<QAction *> topMenus;
    for (QAction *act : bar->actions()) {
        if (act->menu() != nullptr) {
            topMenus.append(act);
        }
    }
    if (topMenus.isEmpty()) {
        return {QStringLiteral("无可用菜单"), false, ActionKind::MenuHoverTiming};
    }

    QAction *chosen = topMenus.at(rng.bounded(topMenus.size()));
    QMenu *menu = chosen->menu();

    // offscreen 下 show() 最可靠，不依赖窗口管理器
    menu->show();
    QTest::qWait(50);
    QCoreApplication::processEvents(QEventLoop::AllEvents);

    if (!menu->isVisible()) {
        return {QStringLiteral("菜单 %1 未弹出").arg(chosen->text()), false, ActionKind::MenuHoverTiming};
    }

    // 用 Esc 关闭菜单（offscreen 下点击窗口别处不触发 QMenu 关闭，Esc 才可靠）
    QElapsedTimer timer;
    timer.start();
    QTest::keyClick(menu, Qt::Key_Escape);

    // 轮询等待关闭，上限 1000ms 防死循环
    while (menu->isVisible() && !timer.hasExpired(1000)) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QTest::qWait(10);
    }
    const qint64 closeMs = timer.elapsed();

    // 兜底：Esc 未关闭时用 hide() 强制收尾
    if (menu->isVisible()) {
        menu->hide();
        QTest::qWait(30);
    }

    ActionResult r{QStringLiteral("菜单 %1 关闭延迟 %2ms").arg(chosen->text()).arg(closeMs),
                  true, ActionKind::MenuHoverTiming};
    r.timingMs = closeMs;
    return r;
}

QString actionKindToString(ActionKind kind)
{
    switch (kind) {
    case ActionKind::TargetRow:
        return QStringLiteral("target_row");
    case ActionKind::Confirm:
        return QStringLiteral("confirm");
    case ActionKind::Start:
        return QStringLiteral("start");
    case ActionKind::Complete:
        return QStringLiteral("complete");
    case ActionKind::CameraTop:
        return QStringLiteral("camera_top");
    case ActionKind::CameraSide:
        return QStringLiteral("camera_side");
    case ActionKind::Camera3D:
        return QStringLiteral("camera_3d");
    case ActionKind::CameraReset:
        return QStringLiteral("camera_reset");
    case ActionKind::TabSwitch:
        return QStringLiteral("tab_switch");
    case ActionKind::Refresh:
        return QStringLiteral("refresh");
    case ActionKind::MenuAction:
        return QStringLiteral("menu_action");
    case ActionKind::ViewToggle:
        return QStringLiteral("view_toggle");
    case ActionKind::SearchInput:
        return QStringLiteral("search_input");
    case ActionKind::NavButton:
        return QStringLiteral("nav_button");
    case ActionKind::MissionRow:
        return QStringLiteral("mission_row");
    case ActionKind::DeviceRow:
        return QStringLiteral("device_row");
    case ActionKind::StatusTab:
        return QStringLiteral("status_tab");
    case ActionKind::KeyTab:
        return QStringLiteral("key_tab");
    case ActionKind::KeyEnter:
        return QStringLiteral("key_enter");
    case ActionKind::KeyEscape:
        return QStringLiteral("key_escape");
    case ActionKind::KeyArrow:
        return QStringLiteral("key_arrow");
    case ActionKind::MenuHoverTiming:
        return QStringLiteral("menu_hover_timing");
    case ActionKind::SearchRobust:
        return QStringLiteral("search_robust");
    }
    return QStringLiteral("unknown");
}

// 覆盖率追踪：记录 (状态, 动作) 对的探索次数，跨轮次持久化
using CoverageMap = QHash<QString, int>;

QString coverageKey(const QString &status, ActionKind kind)
{
    return status + QLatin1Char(':') + actionKindToString(kind);
}

void loadCoverage(const QString &path, CoverageMap &map)
{
    if (path.isEmpty()) {
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        map.insert(it.key(), it.value().toInt());
    }
}

void saveCoverage(const QString &path, const CoverageMap &map)
{
    if (path.isEmpty()) {
        return;
    }
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());
    QJsonObject obj;
    for (auto it = map.begin(); it != map.end(); ++it) {
        obj.insert(it.key(), it.value());
    }
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

// 加权随机选择并执行动作。启用按钮高权重（多点击有效控件），
// 禁用按钮低权重（偶尔验证它们确实无反应），目标行中权重。
// 相机/标签页/刷新为新增动作，中低权重避免喧宾夺主。
// 覆盖率感知：未探索的 (状态,动作) 对权重 ×3，引导探索新路径。
ActionResult pickAndExecuteAction(MainWindow &window, QRandomGenerator &rng,
                                   CoverageMap *coverage = nullptr)
{
    auto *confirm = findWidget<QPushButton>(window, "simulationConfirmButton");
    auto *start = findWidget<QPushButton>(window, "simulationStartButton");
    auto *complete = findWidget<QPushButton>(window, "simulationCompleteButton");
    const QString currentStatus = getPanelStatus(window);

    // Layer 1 扩展：预收集菜单/视图/导航控件，按可用性决定权重
    const QVector<MenuActionInfo> menuActions = collectMenuActions(window);
    const QVector<QAction *> viewActions = collectViewToggleActions(window);
    const QVector<QPushButton *> navButtons = findNavButtons(window);
    QLineEdit *searchEdit = findSearchEdit(window);

    // Layer 3 扩展：预收集任务/设备表与状态子标签页，按可用性决定权重
    auto *missionTable = findMissionTable(window);
    auto *deviceTable = findDeviceTable(window);
    const QVector<QPushButton *> statusTabs = findStatusTabButtons(window);
    const int missionRowCount = (missionTable != nullptr) ? missionTable->rowCount() : 0;
    const int deviceRowCount = (deviceTable != nullptr) ? deviceTable->rowCount() : 0;

    struct Candidate {
        ActionKind kind;
        int baseWeight;
    };
    const QVector<Candidate> cands = {
        {ActionKind::TargetRow, 5},
        {ActionKind::Confirm, (confirm != nullptr && confirm->isEnabled()) ? 10 : 1},
        {ActionKind::Start, (start != nullptr && start->isEnabled()) ? 10 : 1},
        {ActionKind::Complete, (complete != nullptr && complete->isEnabled()) ? 10 : 1},
        {ActionKind::CameraTop, 2},
        {ActionKind::CameraSide, 2},
        {ActionKind::Camera3D, 2},
        {ActionKind::CameraReset, 2},
        {ActionKind::TabSwitch, 3},
        {ActionKind::Refresh, 2},
        {ActionKind::MenuAction, menuActions.isEmpty() ? 0 : 3},
        {ActionKind::ViewToggle, viewActions.isEmpty() ? 0 : 2},
        {ActionKind::SearchInput, (searchEdit == nullptr) ? 0 : 2},
        {ActionKind::NavButton, navButtons.isEmpty() ? 0 : 3},
        {ActionKind::MissionRow, missionRowCount > 0 ? 3 : 0},
        {ActionKind::DeviceRow, deviceRowCount > 0 ? 3 : 0},
        {ActionKind::StatusTab, statusTabs.isEmpty() ? 0 : 2},
        {ActionKind::KeyTab, 2},
        {ActionKind::KeyEnter, 2},
        {ActionKind::KeyEscape, 2},
        {ActionKind::KeyArrow, 2},
        {ActionKind::MenuHoverTiming, 2},
        {ActionKind::SearchRobust, (searchEdit == nullptr) ? 0 : 1},
    };

    struct Weighted {
        ActionKind kind;
        int weight;
    };
    QVector<Weighted> weighted;
    weighted.reserve(cands.size());
    for (const auto &c : cands) {
        int w = c.baseWeight;
        if (coverage != nullptr) {
            const QString key = coverageKey(currentStatus, c.kind);
            const int count = coverage->value(key, 0);
            if (count == 0) {
                w *= 3; // 未探索路径优先
            } else if (count > 3) {
                w = qMax(1, w / 2); // 过度探索的路径降权
            }
        }
        weighted.append({c.kind, w});
    }

    int total = 0;
    for (const auto &w : weighted) {
        total += w.weight;
    }
    if (total <= 0) {
        return {QStringLiteral("无可用动作"), false};
    }
    int r = rng.bounded(total);
    ActionKind chosen = ActionKind::TargetRow;
    for (const auto &w : weighted) {
        r -= w.weight;
        if (r < 0) {
            chosen = w.kind;
            break;
        }
    }

    static const QStringList searchSamples = {
        QStringLiteral("UXO"), QStringLiteral("目标"), QStringLiteral("test"),
        QStringLiteral("12345"), QStringLiteral("不存在的内容xyz")
    };

    // 对抗性输入样本（label 用于 action_log，payload 用于实际注入）
    struct RobustSample { QString label; QString payload; };
    static const RobustSample robustSamples[] = {
        {QStringLiteral("超长文本1000字符"), QStringLiteral("A").repeated(1000)},
        {QStringLiteral("超长文本5000字符"), QStringLiteral("X").repeated(5000)},
        {QStringLiteral("CRLF换行"), QStringLiteral("a\r\nb\r\nc")},
        {QStringLiteral("正则元字符"), QStringLiteral(".*+?^${}()|[]\\")},
        {QStringLiteral("HTML标签"), QStringLiteral("<script>alert(1)</script><b>UXO</b>")},
        {QStringLiteral("emoji"), QStringLiteral("🎯💣🚨目标🎯")},
        {QStringLiteral("RTL阿拉伯文"), QStringLiteral("هدف هدف هدف")},
        {QStringLiteral("null字节"), QStringLiteral("a\x00" "b\x00" "c")},
        {QStringLiteral("制表符"), QStringLiteral("a\tb\tc")},
        {QStringLiteral("混合中日韩"), QStringLiteral("目标UXO배달목표")},
    };
    const int robustCount = sizeof(robustSamples) / sizeof(robustSamples[0]);

    switch (chosen) {
    case ActionKind::TargetRow:
        return clickTargetRow(window, 0); // 当前只有一个演示目标
    case ActionKind::Confirm:
        return clickButton(window, "simulationConfirmButton", QStringLiteral("模拟确认"), chosen);
    case ActionKind::Start:
        return clickButton(window, "simulationStartButton", QStringLiteral("模拟处置"), chosen);
    case ActionKind::Complete:
        return clickButton(window, "simulationCompleteButton", QStringLiteral("模拟完成"), chosen);
    case ActionKind::CameraTop:
        return clickButtonByText(window, QStringLiteral("俯"), QStringLiteral("俯视"), chosen);
    case ActionKind::CameraSide:
        return clickButtonByText(window, QStringLiteral("侧"), QStringLiteral("侧视"), chosen);
    case ActionKind::Camera3D:
        return clickButtonByText(window, QStringLiteral("3D"), QStringLiteral("3D视角"), chosen);
    case ActionKind::CameraReset:
        return clickButtonByText(window, QStringLiteral("复位"), QStringLiteral("相机复位"), chosen);
    case ActionKind::TabSwitch:
        return clickTab(window, rng.bounded(3)); // 3 个标签页随机选一个
    case ActionKind::Refresh:
        return clickButtonByText(window, QStringLiteral("刷新"), QStringLiteral("刷新"), chosen);
    case ActionKind::MenuAction:
        return triggerMenuAction(window, menuActions.at(rng.bounded(menuActions.size())));
    case ActionKind::ViewToggle: {
        const int vi = rng.bounded(viewActions.size());
        return toggleViewAction(window, viewActions.at(vi), vi);
    }
    case ActionKind::SearchInput:
        return inputSearchText(window, searchSamples.at(rng.bounded(searchSamples.size())));
    case ActionKind::SearchRobust: {
        const auto &sample = robustSamples[rng.bounded(robustCount)];
        return inputSearchRobust(window, sample.label, sample.payload);
    }
    case ActionKind::NavButton:
        return clickNavButton(window, rng.bounded(navButtons.size()));
    case ActionKind::MissionRow:
        return clickMissionRow(window, rng.bounded(missionRowCount));
    case ActionKind::DeviceRow:
        return clickDeviceRow(window, rng.bounded(deviceRowCount));
    case ActionKind::StatusTab:
        return clickStatusTab(window, rng.bounded(statusTabs.size()));
    case ActionKind::KeyTab:
        return sendKeyTab(window);
    case ActionKind::KeyEnter:
        return sendKeyEnter(window);
    case ActionKind::KeyEscape:
        return sendKeyEscape(window);
    case ActionKind::KeyArrow:
        return sendKeyArrow(window, rng);
    case ActionKind::MenuHoverTiming:
        return menuHoverTimingAction(window, rng);
    }
    return {QStringLiteral("未知动作"), false};
}

// ===== state 文件写入（崩溃前留痕，供外层读取）=====

void writeStateFile(const QString &path, const QString &description)
{
    if (path.isEmpty()) {
        return;
    }
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream s(&f);
        s.setCodec("UTF-8");
        s << description;
    }
}

// ===== 主程序 =====

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 解析命令行参数
    QString reportPath;
    QString statePath;
    QString screenshotDir;
    QString coveragePath;
    int seed = static_cast<int>(QDateTime::currentDateTime().toMSecsSinceEpoch());
    int maxActions = 20;
    bool selfTest = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLatin1(argv[i]);
        if (arg == QStringLiteral("--report") && i + 1 < argc) {
            reportPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--state") && i + 1 < argc) {
            statePath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--screenshots") && i + 1 < argc) {
            screenshotDir = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--coverage") && i + 1 < argc) {
            coveragePath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--seed") && i + 1 < argc) {
            seed = QString::fromLocal8Bit(argv[++i]).toInt();
        } else if (arg == QStringLiteral("--max-actions") && i + 1 < argc) {
            maxActions = QString::fromLocal8Bit(argv[++i]).toInt();
        } else if (arg == QStringLiteral("--self-test")) {
            selfTest = true;
        } else if (arg == QStringLiteral("--verbose")) {
            verbose = true;
        }
    }

    // verbose 模式用于崩溃复现：每步执行前打印进度（无缓冲），崩溃时最后一行即崩溃点。
    auto verboseLog = [&](const QString &msg) {
        if (verbose) {
            fprintf(stderr, "%s\n", msg.toUtf8().constData());
            fflush(stderr);
        }
    };

    // 创建主窗口（离屏模式由外层 QT_QPA_PLATFORM 环境变量控制）
    auto window = std::make_unique<MainWindow>();
    window->show();
    QTest::qWait(200); // 等待 UI 初始化和首次绘制

    std::vector<Issue> issues;

    // 自测模式：人为制造不一致，验证检查规则能触发。正常巡检不走此分支。
    if (selfTest) {
        // 先选中目标，让三处状态都有值
        clickTargetRow(*window, 0);
        QTest::qWait(100);

        // 人为破坏决策面板状态，制造三处不一致
        auto *decisionLabel = findWidget<QLabel>(*window, "decisionSimulationStatusLabel");
        if (decisionLabel != nullptr) {
            decisionLabel->setText(QStringLiteral("[模拟] 目标状态：处置中"));
        }

        QDir().mkpath(screenshotDir);
        checkStatusConsistency(*window, QStringLiteral("自测：人为破坏决策面板状态"),
                               screenshotDir, 0, issues);

        QTextStream(stderr) << "SELF-TEST: issues=" << issues.size()
                            << " (期望=1，R1应报告三处不一致)\n";
        return issues.size() == 1 ? 0 : 2;
    }

    QRandomGenerator rng(seed);
    QJsonArray actionLog;
    CoverageMap coverage;
    loadCoverage(coveragePath, coverage);

    // 统一执行：动作前快照 -> 动作 -> 动作后检查
    auto runChecks = [&](const QString &action, const Snapshot &pre, int clickedRow,
                         int tabTarget, ActionKind kind, qint64 timingMs,
                         bool dialogShown, const QString &dialogTitle, bool enabledBefore,
                         quintptr focusBeforeAddr, quintptr focusAfterAddr) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QTest::qWait(50);

        const int idx = static_cast<int>(issues.size());
        // 无前置快照依赖的规则
        checkStatusConsistency(*window, action, screenshotDir, idx, issues);
        checkButtonEnableMatchesStatus(*window, action, screenshotDir,
                                       static_cast<int>(issues.size()), issues);
        checkButtonsDisabledWhenNoSelection(*window, action, screenshotDir,
                                            static_cast<int>(issues.size()), issues);
        checkLogMatchesUi(*window, action, screenshotDir,
                          static_cast<int>(issues.size()), issues);
        checkLogFormatConsistency(*window, action, screenshotDir,
                                  static_cast<int>(issues.size()), issues);
        checkPanelTargetIdExists(*window, action, screenshotDir,
                                 static_cast<int>(issues.size()), issues);
        checkTargetRowClickConsistency(*window, clickedRow, action, screenshotDir,
                                       static_cast<int>(issues.size()), issues);
        checkTabSwitchConsistency(*window, tabTarget, action, screenshotDir,
                                  static_cast<int>(issues.size()), issues);
        checkTabPageVisibleAfterSwitch(kind, *window, tabTarget, action, screenshotDir,
                                       static_cast<int>(issues.size()), issues);
        // 依赖前置快照的规则
        checkTransitionLegality(*window, pre, action, screenshotDir,
                                static_cast<int>(issues.size()), issues);
        checkNonStateActionPreservesStatus(kind, *window, pre, action, screenshotDir,
                                           static_cast<int>(issues.size()), issues);
        checkEnabledButtonCausesTransition(kind, enabledBefore, *window, pre, action,
                                           screenshotDir, static_cast<int>(issues.size()), issues);
        checkTabFocusMoved(kind, focusBeforeAddr, focusAfterAddr, *window, action,
                           screenshotDir, static_cast<int>(issues.size()), issues);
        checkTargetReselectIdempotent(*window, pre, clickedRow, action, screenshotDir,
                                      static_cast<int>(issues.size()), issues);
        checkLogLineCountDelta(*window, pre, action, screenshotDir,
                               static_cast<int>(issues.size()), issues);
        checkRefreshPreservesSelection(*window, pre, action, screenshotDir,
                                       static_cast<int>(issues.size()), issues);
        checkCameraDoesNotCorruptState(*window, pre, action, screenshotDir,
                                       static_cast<int>(issues.size()), issues);
        checkWindowStillVisible(*window, action, screenshotDir,
                                static_cast<int>(issues.size()), issues);
        checkViewTogglePreservesState(*window, pre, action, screenshotDir,
                                      static_cast<int>(issues.size()), issues);
        checkSearchPreservesSelection(*window, pre, action, screenshotDir,
                                      static_cast<int>(issues.size()), issues);
        checkTableRowClickPreservesSelection(*window, pre, action, screenshotDir,
                                             static_cast<int>(issues.size()), issues);
        checkStatusTabClickPreservesSelection(*window, pre, action, screenshotDir,
                                              static_cast<int>(issues.size()), issues);
        checkKeyboardNavPreservesSelection(*window, pre, action, screenshotDir,
                                           static_cast<int>(issues.size()), issues);
        checkMenuCloseTiming(timingMs, kind, *window, action, screenshotDir,
                             static_cast<int>(issues.size()), issues);
        checkButtonResponseTiming(timingMs, kind, *window, action, screenshotDir,
                                  static_cast<int>(issues.size()), issues);
        checkTabSwitchTiming(timingMs, kind, *window, action, screenshotDir,
                             static_cast<int>(issues.size()), issues);
        checkSearchFilterTiming(timingMs, kind, *window, action, screenshotDir,
                                static_cast<int>(issues.size()), issues);
        checkSearchRobustTiming(timingMs, kind, *window, action, screenshotDir,
                                static_cast<int>(issues.size()), issues);
        checkDialogTitleNotEmpty(kind, dialogShown, dialogTitle, *window, action,
                                 screenshotDir, static_cast<int>(issues.size()), issues);
        checkNoResidualModalAfterClose(kind, dialogShown, *window, action,
                                       screenshotDir, static_cast<int>(issues.size()), issues);
    };

    // 初始状态检查（无前置动作）
    {
        const Snapshot emptyPre;
        QJsonObject entry;
        entry["step"] = 0;
        entry["action"] = QStringLiteral("初始状态");
        entry["kind"] = QStringLiteral("init");
        entry["executed"] = true;
        actionLog.append(entry);
        runChecks(QStringLiteral("初始状态"), emptyPre, -1, -1, ActionKind::TargetRow, -1,
                  false, QString(), false, 0, 0);
    }

    // R29 会话级泄漏检测基线：记录主循环开始前主窗口的子对象数
    const int initialChildCount = window->children().size();

    // R32 会话级响应时间退化趋势检测：收集每动作耗时
    QVector<qint64> timings;
    timings.reserve(maxActions);

    // 加权随机点击循环
    for (int step = 1; step <= maxActions; ++step) {
        const Snapshot pre = captureSnapshot(*window);
        verboseLog(QStringLiteral("REPLAY step=%1 starting pre=[status:%2 target:%3 loglines:%4 tab:%5]")
                       .arg(step).arg(pre.panelStatus, pre.panelTargetId).arg(pre.logLineCount).arg(pre.tabIndex));
        const ActionResult result = pickAndExecuteAction(*window, rng, &coverage);
        if (result.executed) {
            coverage[coverageKey(pre.panelStatus, result.kind)]++;
        }
        if (result.timingMs >= 0) {
            timings.append(result.timingMs);
        }
        writeStateFile(statePath, result.description);
        verboseLog(QStringLiteral("REPLAY step=%1 done action=%2")
                       .arg(step).arg(result.description));

        QJsonObject entry;
        entry["step"] = step;
        entry["action"] = result.description;
        entry["kind"] = actionKindToString(result.kind);
        entry["executed"] = result.executed;
        if (result.dialogShown) {
            entry["dialog_title"] = result.dialogTitle;
        }
        actionLog.append(entry);

        runChecks(result.description, pre, result.clickedRow, result.tabTarget,
                  result.kind, result.timingMs, result.dialogShown, result.dialogTitle,
                  result.enabledBefore, result.focusBeforeAddr, result.focusAfterAddr);
    }

    saveCoverage(coveragePath, coverage);

    // R29：会话级 widget 泄漏检测（主循环结束后调用一次）
    if (!screenshotDir.isEmpty()) {
        checkSessionWidgetLeak(*window, initialChildCount, screenshotDir,
                               static_cast<int>(issues.size()), issues);
        // R32：会话级响应时间退化趋势检测（主循环结束后调用一次）
        checkSessionResponseTimeDegradation(timings, *window, screenshotDir,
                                            static_cast<int>(issues.size()), issues);
    }

    // 写 JSON 报告
    QJsonObject report;
    report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    report["seed"] = seed;
    report["status"] = QStringLiteral("completed");
    report["actions_executed"] = maxActions;
    report["issues_found"] = static_cast<int>(issues.size());

    QJsonArray issueArray;
    for (const Issue &issue : issues) {
        QJsonObject obj;
        obj["type"] = issue.type;
        obj["rule"] = issue.rule;
        obj["action"] = issue.action;
        obj["details"] = issue.details;
        obj["screenshot"] = issue.screenshot;
        issueArray.append(obj);
    }
    report["issues"] = issueArray;
    report["action_log"] = actionLog;

    if (!reportPath.isEmpty()) {
        const QFileInfo reportInfo(reportPath);
        const QString parentDir = reportInfo.absolutePath();
        if (!parentDir.isEmpty()) {
            QDir().mkpath(parentDir);
        }
        QFile f(reportPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
        }
    }

    // stderr 输出一行摘要，供 run_loop.sh 读取
    QTextStream(stderr) << "SUMMARY: issues=" << issues.size()
                        << " actions=" << maxActions
                        << " seed=" << seed << "\n";

    return 0;
}
