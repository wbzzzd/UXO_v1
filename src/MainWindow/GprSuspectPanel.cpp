#include "MainWindow/GprSuspectPanel.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

// 表格列定义：ID / 道索引 / 深度(m) / 幅度（gpr-display.md §6.2）
constexpr int kColId = 0;          // ID（1 基）
constexpr int kColTraceIndex = 1;  // 道索引
constexpr int kColDepth = 2;       // 深度(m)
constexpr int kColAmplitude = 3;   // 幅度
constexpr int kColCount = 4;

} // namespace

GprSuspectPanel::GprSuspectPanel(QWidget *parent)
    : QWidget(parent)
{
    // objectName 契约（gpr_view UI 契约测试，§7.3）
    setObjectName(QStringLiteral("gprSuspectPanel"));
    setupUi();
}

GprSuspectPanel::~GprSuspectPanel() = default;

void GprSuspectPanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 标题：含 [模拟][GPR自动检测] 模拟与来源标注（§6.4）
    m_titleLabel = new QLabel(QStringLiteral("疑似目标 [模拟][GPR自动检测]"), this);
    m_titleLabel->setWordWrap(true);
    layout->addWidget(m_titleLabel);

    // 候选列表：行号与 m_suspects 下标一一对应，单元格仅存显示文本
    m_table = new QTableWidget(0, kColCount, this);
    m_table->setObjectName(QStringLiteral("gprSuspectTable"));
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"),
        QStringLiteral("道索引"),
        QStringLiteral("深度(m)"),
        QStringLiteral("幅度"),
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table, 1);

    // 操作条：[确认目标] / [标记误报]，初始禁用（§6.3 禁用态）
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    m_confirmBtn = new QPushButton(QStringLiteral("确认目标"), this);
    m_confirmBtn->setObjectName(QStringLiteral("gprConfirmButton"));
    m_confirmBtn->setEnabled(false);
    m_falsePositiveBtn = new QPushButton(QStringLiteral("标记误报"), this);
    m_falsePositiveBtn->setObjectName(QStringLiteral("gprFalsePositiveButton"));
    m_falsePositiveBtn->setEnabled(false);
    buttonRow->addWidget(m_confirmBtn);
    buttonRow->addWidget(m_falsePositiveBtn);
    buttonRow->addStretch();
    layout->addLayout(buttonRow);

    // 选中变化联动按钮状态并向外发选中意图信号
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &GprSuspectPanel::onRowSelectionChanged);
    connect(m_confirmBtn, &QPushButton::clicked,
            this, &GprSuspectPanel::onConfirmClicked);
    connect(m_falsePositiveBtn, &QPushButton::clicked,
            this, &GprSuspectPanel::onFalsePositiveClicked);
}

void GprSuspectPanel::setSuspects(const QList<GPR::GprSuspect> &suspects)
{
    m_suspects = suspects;

    // 重建表格：行号与 m_suspects 下标一一对应
    m_table->clearContents();
    m_table->setRowCount(m_suspects.size());
    for (int row = 0; row < m_suspects.size(); ++row) {
        const GPR::GprSuspect &suspect = m_suspects.at(row);
        m_table->setItem(row, kColId,
                         new QTableWidgetItem(QString::number(suspect.id)));
        m_table->setItem(row, kColTraceIndex,
                         new QTableWidgetItem(QString::number(suspect.traceIndex)));
        m_table->setItem(row, kColDepth,
                         new QTableWidgetItem(QString::number(suspect.depthM, 'f', 2)));
        m_table->setItem(row, kColAmplitude,
                         new QTableWidgetItem(QString::number(suspect.amplitude, 'f', 1)));
    }

    // 清空选中：先重置当前单元格再清选中，
    // 避免 clearSelection() 触发的信号里残留 currentRow 被误判为选中
    m_table->setCurrentCell(-1, -1);
    m_table->clearSelection();
    updateActionState();
}

int GprSuspectPanel::selectedSuspectId() const
{
    // 行号对 m_suspects 越界防护：重建表格过程中选中信号可能先于行数更新触发
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_suspects.size()) {
        return 0;
    }
    return m_suspects.at(row).id;
}

void GprSuspectPanel::onRowSelectionChanged()
{
    updateActionState();
    // 仅在有效选中时向外发意图信号；GprView/MainWindow 消费
    const int suspectId = selectedSuspectId();
    if (suspectId > 0) {
        emit suspectSelected(suspectId);
    }
}

void GprSuspectPanel::updateActionState()
{
    // 空列表或无选中时按钮保持禁用（§6.3 禁用态）
    const bool hasSelection = selectedSuspectId() > 0;
    m_confirmBtn->setEnabled(hasSelection);
    m_falsePositiveBtn->setEnabled(hasSelection);
}

void GprSuspectPanel::onConfirmClicked()
{
    const int suspectId = selectedSuspectId();
    if (suspectId > 0) {
        emit confirmRequested(suspectId);
    }
}

void GprSuspectPanel::onFalsePositiveClicked()
{
    const int suspectId = selectedSuspectId();
    if (suspectId > 0) {
        emit falsePositiveRequested(suspectId);
    }
}
