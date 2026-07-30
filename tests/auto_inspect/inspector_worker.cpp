// 巡检员 worker：单轮随机点击 + 自洽性检查。
// 独立进程运行，每轮由 run_loop.sh 启动，崩溃/卡死由外层 timeout 检测。
// 设计原则：只读检查，不修改任何业务代码；复用现有测试的离屏启动方式。
//
// 检查规则（共10条，均为自洽性检查，不依赖业务知识）：
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

#include "MainWindow/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTest>
#include <QTextStream>
#include <QTextEdit>

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
};

Snapshot captureSnapshot(MainWindow &window)
{
    return {getPanelStatus(window), getPanelTargetId(window), getLogLineCount(window)};
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

// ===== 动作执行 =====

enum class ActionKind {
    TargetRow,
    Confirm,
    Start,
    Complete,
};

struct ActionResult {
    QString description;
    bool executed;
    ActionKind kind = ActionKind::TargetRow;
    int clickedRow = -1; // 仅 TargetRow 有效
};

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
    QTest::mouseClick(btn, Qt::LeftButton);
    return {QStringLiteral("点击 %1 按钮").arg(displayName), true, kind};
}

// 加权随机选择并执行动作。启用按钮高权重（多点击有效控件），
// 禁用按钮低权重（偶尔验证它们确实无反应），目标行中权重。
ActionResult pickAndExecuteAction(MainWindow &window, QRandomGenerator &rng)
{
    auto *confirm = findWidget<QPushButton>(window, "simulationConfirmButton");
    auto *start = findWidget<QPushButton>(window, "simulationStartButton");
    auto *complete = findWidget<QPushButton>(window, "simulationCompleteButton");

    struct Candidate {
        ActionKind kind;
        int weight;
    };
    const QVector<Candidate> cands = {
        {ActionKind::TargetRow, 5},
        {ActionKind::Confirm, (confirm != nullptr && confirm->isEnabled()) ? 10 : 1},
        {ActionKind::Start, (start != nullptr && start->isEnabled()) ? 10 : 1},
        {ActionKind::Complete, (complete != nullptr && complete->isEnabled()) ? 10 : 1},
    };

    int total = 0;
    for (const auto &c : cands) {
        total += c.weight;
    }
    int r = rng.bounded(total);
    ActionKind chosen = ActionKind::TargetRow;
    for (const auto &c : cands) {
        r -= c.weight;
        if (r < 0) {
            chosen = c.kind;
            break;
        }
    }

    switch (chosen) {
    case ActionKind::TargetRow:
        return clickTargetRow(window, 0); // 当前只有一个演示目标
    case ActionKind::Confirm:
        return clickButton(window, "simulationConfirmButton", QStringLiteral("模拟确认"), chosen);
    case ActionKind::Start:
        return clickButton(window, "simulationStartButton", QStringLiteral("模拟处置"), chosen);
    case ActionKind::Complete:
        return clickButton(window, "simulationCompleteButton", QStringLiteral("模拟完成"), chosen);
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

// 把 ActionKind 序列化为字符串，写入报告便于聚合分析
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
    }
    return QStringLiteral("unknown");
}

// ===== 主程序 =====

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 解析命令行参数
    QString reportPath;
    QString statePath;
    QString screenshotDir;
    int seed = static_cast<int>(QDateTime::currentDateTime().toMSecsSinceEpoch());
    int maxActions = 20;
    bool selfTest = false;

    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLatin1(argv[i]);
        if (arg == QStringLiteral("--report") && i + 1 < argc) {
            reportPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--state") && i + 1 < argc) {
            statePath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--screenshots") && i + 1 < argc) {
            screenshotDir = QString::fromLocal8Bit(argv[++i]);
        } else if (arg == QStringLiteral("--seed") && i + 1 < argc) {
            seed = QString::fromLocal8Bit(argv[++i]).toInt();
        } else if (arg == QStringLiteral("--max-actions") && i + 1 < argc) {
            maxActions = QString::fromLocal8Bit(argv[++i]).toInt();
        } else if (arg == QStringLiteral("--self-test")) {
            selfTest = true;
        }
    }

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

    // 统一执行：动作前快照 -> 动作 -> 动作后检查
    auto runChecks = [&](const QString &action, const Snapshot &pre, int clickedRow) {
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
        // 依赖前置快照的规则
        checkTransitionLegality(*window, pre, action, screenshotDir,
                                static_cast<int>(issues.size()), issues);
        checkTargetReselectIdempotent(*window, pre, clickedRow, action, screenshotDir,
                                      static_cast<int>(issues.size()), issues);
        checkLogLineCountDelta(*window, pre, action, screenshotDir,
                               static_cast<int>(issues.size()), issues);
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
        runChecks(QStringLiteral("初始状态"), emptyPre, -1);
    }

    // 加权随机点击循环
    for (int step = 1; step <= maxActions; ++step) {
        const Snapshot pre = captureSnapshot(*window);
        const ActionResult result = pickAndExecuteAction(*window, rng);
        writeStateFile(statePath, result.description);

        QJsonObject entry;
        entry["step"] = step;
        entry["action"] = result.description;
        entry["kind"] = actionKindToString(result.kind);
        entry["executed"] = result.executed;
        actionLog.append(entry);

        runChecks(result.description, pre, result.clickedRow);
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
