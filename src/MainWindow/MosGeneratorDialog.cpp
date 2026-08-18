// MOS-015 模拟损毁分布生成器模态实现：种子化本地随机生成弹坑与 UXO 分布参数表单。
// 不联网、不持久化到数据库；JSON 按钮直接调用 controller 导出实时生成 fixture。
// 所有参数均为合成本地 fixture 语义，非真实探测或真实装药。

#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosPlanningController.h"
#include "Common/GlobalStyle.h"
#include "Core/MOS/MosValidation.h"

#include <QDir>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <climits>

MosGeneratorDialog::MosGeneratorDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("DEC-GEN-MODAL"));
    setWindowTitle(QStringLiteral("模拟损毁分布生成器 [模拟 · MOS-015]"));
    setModal(true);
    setupUi();
    revalidate();
}

MosGeneratorDialog::~MosGeneratorDialog() = default;

void MosGeneratorDialog::setupUi()
{
    resize(520, 360);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    // 行间距收紧到 6：为独立的模拟说明行留出垂直空间，保证 520x360 参考尺寸下不裁剪
    root->setSpacing(6);

    // 头部：标题 + 关闭按钮
    auto *header = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("模拟损毁分布生成器 [模拟 · MOS-015]"), this);
    QFont tf = title->font();
    tf.setBold(true);
    title->setFont(tf);
    header->addWidget(title);
    header->addStretch();
    auto *closeBtn = new QPushButton(QStringLiteral("×"), this);
    closeBtn->setObjectName(QStringLiteral("DEC-GEN-CLOSE"));
    closeBtn->setFixedSize(28, 28);
    header->addWidget(closeBtn);
    root->addLayout(header);

    // 表单：弹坑参数 + UXO 参数 + 种子
    auto *form = new QFormLayout();
    // 表单行距收紧到 4：配合 root 间距缩减，为新增模拟说明行腾出垂直空间
    form->setSpacing(4);
    auto mkInt = [&](int min, int max, int val) {
        auto *sb = new QSpinBox(this);
        sb->setRange(min, max);
        sb->setValue(val);
        sb->setFixedHeight(26);
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](){ revalidate(); });
        return sb;
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

    m_craterCount = mkInt(1, 8, 2);
    m_craterCount->setObjectName(QStringLiteral("DEC-GEN-CRATER-COUNT"));
    form->addRow(QStringLiteral("弹坑数量"), m_craterCount);
    m_craterRMin = mkDouble(0.1, 100.0, 2, 3.0);
    m_craterRMin->setObjectName(QStringLiteral("DEC-GEN-CRATER-RMIN"));
    form->addRow(QStringLiteral("半径最小 (m)"), m_craterRMin);
    m_craterRMax = mkDouble(0.1, 100.0, 2, 6.0);
    m_craterRMax->setObjectName(QStringLiteral("DEC-GEN-CRATER-RMAX"));
    form->addRow(QStringLiteral("半径最大 (m)"), m_craterRMax);
    m_uxoCount = mkInt(0, 5, 2);
    m_uxoCount->setObjectName(QStringLiteral("DEC-GEN-UXO-COUNT"));
    form->addRow(QStringLiteral("UXO 数量"), m_uxoCount);
    m_uxoYMin = mkDouble(0.0, 10000.0, 2, 10.0);
    m_uxoYMin->setObjectName(QStringLiteral("DEC-GEN-UXO-YMIN"));
    form->addRow(QStringLiteral("当量最小 (kg)"), m_uxoYMin);
    m_uxoYMax = mkDouble(0.0, 10000.0, 2, 50.0);
    m_uxoYMax->setObjectName(QStringLiteral("DEC-GEN-UXO-YMAX"));
    form->addRow(QStringLiteral("当量最大 (kg)"), m_uxoYMax);
    m_seed = new QSpinBox(this);
    m_seed->setRange(INT_MIN, INT_MAX);
    m_seed->setValue(42);
    m_seed->setFixedHeight(26);
    m_seed->setObjectName(QStringLiteral("DEC-GEN-SEED"));
    connect(m_seed, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](){ revalidate(); });
    form->addRow(QStringLiteral("随机种子"), m_seed);
    root->addLayout(form);

    // 校验横幅 DEC-GEN-BANNER
    m_banner = new QLabel(this);
    m_banner->setObjectName(QStringLiteral("DEC-GEN-BANNER"));
    m_banner->setWordWrap(true);
    m_banner->setFixedHeight(28);
    root->addWidget(m_banner);

    // 模拟说明：独立换行行，置于按钮行之上，避免与操作按钮挤占水平空间
    auto *note = new QLabel(QStringLiteral("[模拟] 当量为模拟处理假设，非真实装药"), this);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:%1;").arg(GlobalStyle::Colors::TextSecondary));
    root->addWidget(note);

    // 底部按钮行：JSON/取消/应用 单独成行，右对齐保持与原布局视觉一致
    auto *bottom = new QHBoxLayout();
    bottom->addStretch();
    auto *jsonBtn = new QPushButton(QStringLiteral("⬇ 下载模拟场景 JSON"), this);
    jsonBtn->setObjectName(QStringLiteral("DEC-GEN-JSON"));
    bottom->addWidget(jsonBtn);
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    cancelBtn->setObjectName(QStringLiteral("DEC-GEN-CANCEL"));
    bottom->addWidget(cancelBtn);
    m_applyBtn = new QPushButton(QStringLiteral("应用生成"), this);
    m_applyBtn->setObjectName(QStringLiteral("DEC-GEN-APPLY"));
    // 统一使用 token 替代 CSS 命名色 white，与 RightPanelWidget 等文件保持一致
    m_applyBtn->setStyleSheet(QStringLiteral("background:%1; color:%2; padding:4px 12px;")
                                  .arg(GlobalStyle::Colors::PrimaryGreen, GlobalStyle::Colors::TextPrimary));
    bottom->addWidget(m_applyBtn);
    root->addLayout(bottom);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(jsonBtn, &QPushButton::clicked, this, [this](){ doExportJson(); });
    connect(m_applyBtn, &QPushButton::clicked, this, [this](){ onApply(); });
}

Core::MOS::MosGeneratorParams MosGeneratorDialog::currentParams() const
{
    Core::MOS::MosGeneratorParams p;
    p.craterCount = m_craterCount->value();
    p.craterRMin = m_craterRMin->value();
    p.craterRMax = m_craterRMax->value();
    p.uxoCount = m_uxoCount->value();
    p.uxoYMin = m_uxoYMin->value();
    p.uxoYMax = m_uxoYMax->value();
    return p;
}

qint32 MosGeneratorDialog::currentSeed() const
{
    return static_cast<qint32>(m_seed->value());
}

void MosGeneratorDialog::setController(Core::MOS::MosPlanningController *controller)
{
    m_controller = controller;
}

void MosGeneratorDialog::setRunwayParams(const Core::MOS::MosRunwayParams &params)
{
    m_runwayParams = params;
}

void MosGeneratorDialog::revalidate()
{
    const auto params = currentParams();
    const auto seedResult = Core::MOS::validateSeed(static_cast<qint64>(currentSeed()));
    const auto paramsResult = Core::MOS::validateGeneratorParams(params);
    const bool valid = paramsResult.valid && seedResult.valid;
    if (valid) {
        m_banner->setText(QStringLiteral("参数校验通过 · 弹坑%1 + UXO%2 · 种子%3")
                              .arg(params.craterCount).arg(params.uxoCount).arg(currentSeed()));
        m_banner->setStyleSheet(QStringLiteral("background:%1; color:%2;")
                                    .arg(GlobalStyle::Colors::StatusOnline, GlobalStyle::Colors::TextPrimary));
    } else {
        QString msg = !paramsResult.valid ? paramsResult.message : seedResult.message;
        m_banner->setText(QStringLiteral("参数校验失败：%1").arg(msg));
        m_banner->setStyleSheet(QStringLiteral("background:%1; color:%2;")
                                    .arg(GlobalStyle::Colors::ThreatHigh, GlobalStyle::Colors::TextPrimary));
    }
    m_applyBtn->setEnabled(valid);
}

void MosGeneratorDialog::doExportJson()
{
    if (!m_controller) return;
    const qint32 seed = currentSeed();
    const QString fileName = QStringLiteral("mos-sim-scenario-seed%1-prototype.json").arg(seed);
    const QString path = QDir::currentPath() + QDir::separator() + fileName;
    m_controller->exportFixture(path, m_runwayParams, currentParams(), seed);
}

void MosGeneratorDialog::onApply()
{
    if (!m_applyBtn->isEnabled()) return;
    emit applied(currentParams(), currentSeed());
    accept();
}
