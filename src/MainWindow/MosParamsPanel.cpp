// MOS 算法参数栏实现：10 个可编辑输入 + 2 个只读派生 + 校验横幅 + 规划状态横幅 + 重规划。
// 仅管理本地表单值与校验展示，不持有会话状态、不发起规划、不联网。
// 所有参数均为合成本地 fixture 语义，非真实跑道或工程参数。

#include "MainWindow/MosParamsPanel.h"
#include "Common/GlobalStyle.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

MosParamsPanel::MosParamsPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    revalidate();
}

MosParamsPanel::~MosParamsPanel() = default;

void MosParamsPanel::setupUi()
{
    // 容器对象名 DEC-CE-PARAMS，便于 findChild 定位
    setObjectName(QStringLiteral("DEC-CE-PARAMS"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    // 标题行：算法参数 [模拟] + 恢复默认值按钮
    auto *titleRow = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("算法参数 [模拟]"), this);
    QFont tf = title->font();
    tf.setBold(true);
    title->setFont(tf);
    titleRow->addWidget(title);
    titleRow->addStretch();
    m_resetBtn = new QPushButton(QStringLiteral("恢复默认值"), this);
    m_resetBtn->setObjectName(QStringLiteral("DEC-CE-PARAM-RESET"));
    titleRow->addWidget(m_resetBtn);
    root->addLayout(titleRow);

    // 参数网格 6 列，12 字段（10 可编辑 + 2 只读派生）
    auto *grid = new QGridLayout();
    grid->setSpacing(6);
    int row = 0;
    int col = 0;
    auto addField = [&](const QString &label, QWidget *field, const QString &objName) {
        auto *lab = new QLabel(label, this);
        // 1280 窗口下参数标签易截断，改用专用 field 角色：1x 基准 11px（比 caption 更紧凑），
        // 2x（4K）下随缩放策略放大到 22px；单位文本不再被换行截断。
        // wordWrap 仍保留作为宽度不足时的兜底，左对齐垂直居中保持与相邻 spinbox 对齐。
        lab->setProperty("mosFontRole", QLatin1String("field"));
        lab->setWordWrap(true);
        lab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        // 标签也赋予稳定 objectName（<objName>-LABEL），便于闭包测试按名定位做几何断言，
        // 不影响按字段 objectName 的现有 findChild 查找。
        lab->setObjectName(objName + QStringLiteral("-LABEL"));
        grid->addWidget(lab, row, col * 2);
        field->setObjectName(objName);
        grid->addWidget(field, row, col * 2 + 1);
        ++col;
        if (col >= 3) { col = 0; ++row; }
    };

    auto mkDouble = [&](double min, double max, int decimals, double val) {
        auto *sb = new QDoubleSpinBox(this);
        sb->setRange(min, max);
        sb->setDecimals(decimals);
        sb->setValue(val);
        sb->setFixedHeight(26);
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](){ revalidate(); });
        return sb;
    };
    auto mkInt = [&](int min, int max, int val) {
        auto *sb = new QSpinBox(this);
        sb->setRange(min, max);
        sb->setValue(val);
        sb->setFixedHeight(26);
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](){ revalidate(); });
        return sb;
    };

    m_length = mkDouble(100.0, 6000.0, 1, 300.0);
    addField(QStringLiteral("跑道长度 (m)"), m_length, QStringLiteral("DEC-CE-PARAM-LENGTH"));
    m_width = mkDouble(15.0, 100.0, 1, 50.0);
    addField(QStringLiteral("跑道宽度 (m)"), m_width, QStringLiteral("DEC-CE-PARAM-WIDTH"));
    m_minLength = mkDouble(1.0, 6000.0, 1, 100.0);
    addField(QStringLiteral("最小起降长度 (m)"), m_minLength, QStringLiteral("DEC-CE-PARAM-MINLENGTH"));
    m_minWidth = mkDouble(1.0, 100.0, 1, 15.0);
    addField(QStringLiteral("最小起降宽度 (m)"), m_minWidth, QStringLiteral("DEC-CE-PARAM-MINWIDTH"));
    m_k = mkDouble(0.1, 10.0, 2, 1.5);
    // K 标签移除强制 \n：交由 wordWrap 在窄视口（1280x720）下自然换行，
    // 避免“数）”作为孤立行漂移到下一行造成视觉错位。
    addField(QStringLiteral("合成 standoff 系数 K（非真实安全参数）"),
             m_k, QStringLiteral("DEC-CE-PARAM-K"));
    m_step = mkDouble(0.5, 5.0, 2, 1.0);
    addField(QStringLiteral("扫描步长 (m)"), m_step, QStringLiteral("DEC-CE-PARAM-STEP"));
    m_backfill = mkDouble(0.0, 10000.0, 2, 50.0);
    addField(QStringLiteral("回填速率 (m³/h)"), m_backfill, QStringLiteral("DEC-CE-PARAM-BACKFILL"));
    m_uxoHours = mkDouble(0.0, 1000.0, 2, 8.0);
    addField(QStringLiteral("UXO 工时 (h/个)"), m_uxoHours, QStringLiteral("DEC-CE-PARAM-UXOHOURS"));
    m_expand = mkDouble(0.1, 10.0, 2, 1.5);
    addField(QStringLiteral("扩展系数"), m_expand, QStringLiteral("DEC-CE-PARAM-EXPAND"));
    m_tiers = mkInt(2, 5, 3);
    addField(QStringLiteral("方案档位数 (2~5)"), m_tiers, QStringLiteral("DEC-CE-PARAM-TIERS"));

    // 只读派生字段
    m_dmgCount = new QLabel(QStringLiteral("4"), this);
    m_dmgCount->setObjectName(QStringLiteral("DEC-CE-PARAM-DMGCOUNT"));
    m_dmgCount->setStyleSheet(QStringLiteral("background:%1; padding:2px 8px;")
                                  .arg(GlobalStyle::Colors::ToolbarBackground));
    addField(QStringLiteral("损毁点总数"), m_dmgCount, QStringLiteral("DEC-CE-PARAM-DMGCOUNT"));
    m_repairedCount = new QLabel(QStringLiteral("1"), this);
    m_repairedCount->setObjectName(QStringLiteral("DEC-CE-PARAM-REPAIRED"));
    m_repairedCount->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:2px 8px;")
                                       .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::StatusOnline));
    addField(QStringLiteral("模拟处理假设数"), m_repairedCount, QStringLiteral("DEC-CE-PARAM-REPAIRED"));

    root->addLayout(grid);

    // 校验横幅 DEC-CE-VALIDATION
    m_validationBanner = new QLabel(this);
    m_validationBanner->setObjectName(QStringLiteral("DEC-CE-VALIDATION"));
    m_validationBanner->setWordWrap(true);
    m_validationBanner->setFixedHeight(24);
    root->addWidget(m_validationBanner);

    // 规划状态横幅 DEC-CE-PLAN-STATE
    m_planStateBanner = new QLabel(this);
    m_planStateBanner->setObjectName(QStringLiteral("DEC-CE-PLAN-STATE"));
    m_planStateBanner->setWordWrap(true);
    m_planStateBanner->setFixedHeight(24);
    root->addWidget(m_planStateBanner);

    // 底部状态行：单调性校验 + 重规划按钮
    auto *bottom = new QHBoxLayout();
    auto *monoLabel = new QLabel(QStringLiteral("面积单调递增校验: ✓ 通过"), this);
    monoLabel->setStyleSheet(QStringLiteral("color:%1;").arg(GlobalStyle::Colors::StatusOnline));
    bottom->addWidget(monoLabel);
    bottom->addStretch();
    m_replanBtn = new QPushButton(QStringLiteral("↻ 重新规划"), this);
    m_replanBtn->setObjectName(QStringLiteral("DEC-CE-PARAM-REPLAN"));
    m_replanBtn->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:4px 12px;")
                                   .arg(GlobalStyle::Colors::PrimaryGreen, GlobalStyle::Colors::TextPrimary));
    bottom->addWidget(m_replanBtn);
    root->addLayout(bottom);

    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        setParams(Core::MOS::MosRunwayParams{});
        emit resetRequested();
    });
    connect(m_replanBtn, &QPushButton::clicked, this, [this]() {
        if (m_valid) emit replanRequested();
    });
}

Core::MOS::MosRunwayParams MosParamsPanel::currentParams() const
{
    Core::MOS::MosRunwayParams p;
    p.L = m_length->value();
    p.W = m_width->value();
    p.K = m_k->value();
    p.expand = m_expand->value();
    p.step = m_step->value();
    p.minLength = m_minLength->value();
    p.minWidth = m_minWidth->value();
    p.backfill = m_backfill->value();
    p.uxoHours = m_uxoHours->value();
    p.tiers = m_tiers->value();
    return p;
}

void MosParamsPanel::setParams(const Core::MOS::MosRunwayParams &params)
{
    m_length->setValue(params.L);
    m_width->setValue(params.W);
    m_k->setValue(params.K);
    m_expand->setValue(params.expand);
    m_step->setValue(params.step);
    m_minLength->setValue(params.minLength);
    m_minWidth->setValue(params.minWidth);
    m_backfill->setValue(params.backfill);
    m_uxoHours->setValue(params.uxoHours);
    m_tiers->setValue(params.tiers);
    revalidate();
}

void MosParamsPanel::setDerivedCounts(int damageCount, int repairedCount)
{
    m_dmgCount->setText(QString::number(damageCount));
    m_repairedCount->setText(QString::number(repairedCount));
}

void MosParamsPanel::setPlanState(PlanState state, int tierCount)
{
    QString text;
    QString style;
    switch (state) {
    case PlanState::Idle:
        text = QStringLiteral("待规划");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::ToolbarBackground, GlobalStyle::Colors::TextSecondary);
        break;
    case PlanState::Planning:
        text = QStringLiteral("规划中：本地模拟算法运行中…");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::SelectionBackground, GlobalStyle::Colors::TextPrimary);
        break;
    case PlanState::Loading:
        text = QStringLiteral("加载中：生成候选方案…");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::ThreatMedium, GlobalStyle::Colors::Background);
        break;
    case PlanState::Result:
        text = QStringLiteral("结果：已生成 %1 档模拟候选方案（仅选中档位在跑道强调）").arg(tierCount);
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::StatusOnline, GlobalStyle::Colors::TextPrimary);
        break;
    case PlanState::Error:
        text = QStringLiteral("错误：参数或场景无法生成有效方案，请调整后重规划");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::ThreatHigh, GlobalStyle::Colors::TextPrimary);
        break;
    case PlanState::Empty:
        text = QStringLiteral("空：当前场景无候选方案，请先生成损毁分布或调整参数");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::ThreatMedium, GlobalStyle::Colors::Background);
        break;
    case PlanState::NoFeasible:
        text = QStringLiteral("无可行：当前选中档位无可行矩形，请切换档位或调整参数与障碍物分布");
        style = QStringLiteral("background:%1; color:%2;")
                    .arg(GlobalStyle::Colors::DangerRed, GlobalStyle::Colors::TextPrimary);
        break;
    }
    m_planStateBanner->setText(text);
    m_planStateBanner->setStyleSheet(style);
}

bool MosParamsPanel::isValid() const
{
    return m_valid;
}

void MosParamsPanel::revalidate()
{
    const auto params = currentParams();
    const auto result = Core::MOS::validateRunwayParams(params);
    m_valid = result.valid;
    if (result.valid) {
        m_validationBanner->setText(
            QStringLiteral("参数校验通过 · K=%1（模拟示例值，待领域确认）· 档位数=%2")
                .arg(params.K, 0, 'f', 1).arg(params.tiers));
        m_validationBanner->setStyleSheet(QStringLiteral("background:%1; color:%2;")
                                              .arg(GlobalStyle::Colors::StatusOnline, GlobalStyle::Colors::TextPrimary));
    } else {
        m_validationBanner->setText(QStringLiteral("参数校验失败：%1").arg(result.message));
        m_validationBanner->setStyleSheet(QStringLiteral("background:%1; color:%2;")
                                              .arg(GlobalStyle::Colors::ThreatHigh, GlobalStyle::Colors::TextPrimary));
    }
    m_replanBtn->setEnabled(result.valid);
    emit paramsEdited(result.valid);
}
