#include "MainWindow/TargetDetailOverlay.h"

#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDateTime>
#include <QPixmap>
#include <QtMath>
#include <QResizeEvent>

TargetDetailOverlay::TargetDetailOverlay(QWidget *parent)
    : QWidget(parent)
    , m_hasTarget(false)
    , m_hasEvidence(false)
    , m_evidenceContainer(nullptr)
    , m_evidenceViewport(nullptr)
    , m_evidenceImageLabel(nullptr)
    , m_evidencePlaceholder(nullptr)
    , m_frozenChip(nullptr)
    , m_idLabel(nullptr)
    , m_typeLabel(nullptr)
    , m_threatValue(nullptr)
    , m_confValue(nullptr)
    , m_coordValue(nullptr)
    , m_deviceValue(nullptr)
    , m_distValue(nullptr)
    , m_captureTimeValue(nullptr)
    , m_videoTimeValue(nullptr)
    , m_provenanceValue(nullptr)
    , m_detailContainer(nullptr)
    , m_actionsContainer(nullptr)
    , m_pendingMsg(nullptr)
    , m_feedback(nullptr)
    , m_closeBtn(nullptr)
    , m_createTaskBtn(nullptr)
    , m_assignDeviceBtn(nullptr)
    , m_viewHistoryBtn(nullptr)
{
    setupUi();
    // 默认隐藏，选中目标时由 showTarget 显现
    hide();
}

TargetDetailOverlay::~TargetDetailOverlay() = default;

void TargetDetailOverlay::setupUi()
{
    // 固定 340px 宽，不透明深色面板（PanelBackground），避免地图透显
    // 属性转换：根容器背景交由 containerBg="panel"（=PanelBackground）词汇表规则；
    // 剩余 border(1px solid Border) 与 border-radius(3px ≠ cardRadius 的 4px) 保留内联。
    // WA_StyledBackground 紧跟 setProperty：plain QWidget 需显式开启才能绘制 QSS 背景，
    // 否则地图会从浮层整体透显。
    setProperty("containerBg", "panel");
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("targetDetailOverlay"));
    setFixedWidth(340);
    setStyleSheet(QString(
        "TargetDetailOverlay { border: 1px solid %1; border-radius: 3px; }")
        .arg(GlobalStyle::Colors::Border));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    // 关闭按钮（绝对定位右上角）
    m_closeBtn = new QPushButton(QStringLiteral("✕"), this);
    m_closeBtn->setObjectName(QStringLiteral("targetDetailCloseButton"));
    m_closeBtn->setFixedSize(22, 22);
    m_closeBtn->setStyleSheet(QString(
        "QPushButton { color: %1; border: none; border-radius: 3px; font-size: 14px; }"
        "QPushButton:hover { background-color: %2; color: %3; }")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::DangerRed)
        .arg(GlobalStyle::Colors::TextPrimary));
    connect(m_closeBtn, &QPushButton::clicked, this, &TargetDetailOverlay::onCloseClicked);

    // === 证据图像视口（冻结标注截图，详情浮层最焦点内容） ===
    // DESIGN.md §5.3：316x180 视口，#161616 底，1px Border，3px 圆角。
    // token EvidenceViewport/EvidenceViewportHeight 尚未登记到 GlobalStyle（本任务不改 GlobalStyle），
    // 此处暂用字面量，登记后替换。
    m_evidenceContainer = new QWidget(this);
    m_evidenceContainer->setObjectName(QStringLiteral("targetDetailEvidenceContainer"));
    auto *evidenceLayout = new QVBoxLayout(m_evidenceContainer);
    evidenceLayout->setContentsMargins(0, 0, 0, 0);
    evidenceLayout->setSpacing(4);

    m_evidenceViewport = new QWidget(m_evidenceContainer);
    m_evidenceViewport->setObjectName(QStringLiteral("targetDetailEvidenceViewport"));
    m_evidenceViewport->setMinimumHeight(180);
    // 视口暗底使用 ViewportDark token（视频/预览区域专用暗底）
    m_evidenceViewport->setStyleSheet(QString(
        "background-color: %1; border: 1px solid %2; border-radius: 3px;")
        .arg(GlobalStyle::Colors::ViewportDark)
        .arg(GlobalStyle::Colors::Border));
    auto *viewportLayout = new QVBoxLayout(m_evidenceViewport);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setAlignment(Qt::AlignCenter);

    m_evidenceImageLabel = new QLabel(m_evidenceViewport);
    m_evidenceImageLabel->setObjectName(QStringLiteral("targetDetailEvidenceImage"));
    m_evidenceImageLabel->setAlignment(Qt::AlignCenter);
    m_evidenceImageLabel->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    m_evidenceImageLabel->hide();
    viewportLayout->addWidget(m_evidenceImageLabel);

    m_evidencePlaceholder = new QLabel(m_evidenceViewport);
    m_evidencePlaceholder->setObjectName(QStringLiteral("targetDetailEvidencePlaceholder"));
    m_evidencePlaceholder->setAlignment(Qt::AlignCenter);
    m_evidencePlaceholder->setStyleSheet(QString("color: %1; font-size: %2px; border: none; background: transparent;")
        .arg(GlobalStyle::Colors::TextDisabled)
        .arg(GlobalStyle::Fonts::CaptionSize));
    m_evidencePlaceholder->setText(QStringLiteral("暂无证据快照[模拟]"));
    viewportLayout->addWidget(m_evidencePlaceholder);

    // 冻结标识 chip（绝对定位左上角）
    m_frozenChip = new QLabel(m_evidenceViewport);
    m_frozenChip->setObjectName(QStringLiteral("targetDetailEvidenceFrozenChip"));
    m_frozenChip->setStyleSheet(QString(
        "color: %1; font-size: %2px; padding: 2px 6px; border-radius: 3px; "
        "background-color: rgba(37, 37, 38, 200); border: none;")
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Fonts::CaptionSize));
    m_frozenChip->setText(QStringLiteral("证据快照（已冻结）"));
    m_frozenChip->adjustSize();
    m_frozenChip->move(4, 4);
    m_frozenChip->raise();

    evidenceLayout->addWidget(m_evidenceViewport);

    // 证据元数据行：捕获时间 / 视频时间 / 来源
    auto makeEvidenceRow = [this](const QString &labelText, QLabel *&valueLabel, const QString &valueObjectName) {
        auto *row = new QWidget(m_evidenceContainer);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        auto *label = new QLabel(labelText, row);
        // 属性转换：证据行标签交由 labelRole="caption"（TextSecondary/CaptionSize 12px/transparent）；
        // 原 border:none 为 QLabel 默认，完整移除内联样式
        label->setProperty("labelRole", "caption");
        label->setFixedWidth(64);
        rowLayout->addWidget(label);
        valueLabel = new QLabel(row);
        valueLabel->setObjectName(valueObjectName);
        valueLabel->setStyleSheet(QString("color: %1; font-size: %2px; border: none;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::CaptionSize));
        rowLayout->addWidget(valueLabel, 1);
        return row;
    };
    evidenceLayout->addWidget(makeEvidenceRow(QStringLiteral("捕获时间"), m_captureTimeValue, QStringLiteral("targetDetailCaptureTimeValue")));
    evidenceLayout->addWidget(makeEvidenceRow(QStringLiteral("视频时间"), m_videoTimeValue, QStringLiteral("targetDetailVideoTimeValue")));
    evidenceLayout->addWidget(makeEvidenceRow(QStringLiteral("来源"), m_provenanceValue, QStringLiteral("targetDetailProvenanceValue")));

    rootLayout->addWidget(m_evidenceContainer);

    // 头部：独立容器配 ToolbarBackground 不透明底，与 PanelBackground 面板形成明度阶差，
    // 避免地图从头部行透显
    auto *headerWidget = new QWidget(this);
    // 属性转换：头部底色交由 containerBg="toolbar"（=ToolbarBackground）词汇表规则；
    // 原 border:none 为 plain QWidget 默认（全局无 QWidget border 规则），完整移除内联样式。
    // WA_StyledBackground 使头部 plain QWidget 绘制 ToolbarBackground 底色，与浮层根形成明度阶差。
    headerWidget->setProperty("containerBg", "toolbar");
    headerWidget->setAttribute(Qt::WA_StyledBackground, true);
    headerWidget->setObjectName(QStringLiteral("targetDetailHeader"));
    auto *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setSpacing(8);
    headerLayout->setContentsMargins(0, 0, 24, 0);  // 右侧给关闭按钮留位
    m_idLabel = new QLabel(headerWidget);
    // 属性转换：ID 标题交由 labelRole="h1"（TextPrimary/TitleSize 16px/bold/transparent）词汇表规则；
    // 原 border:none 为 QLabel 默认（全局 QLabel 规则无 border），完整移除内联样式
    m_idLabel->setProperty("labelRole", "h1");
    m_idLabel->setObjectName(QStringLiteral("targetDetailIdLabel"));
    headerLayout->addWidget(m_idLabel);

    m_typeLabel = new QLabel(headerWidget);
    m_typeLabel->setObjectName(QStringLiteral("targetDetailTypeLabel"));
    m_typeLabel->setStyleSheet(QString("color: %1; font-size: %2px; padding: 2px 8px; "
        "border-radius: 3px; background-color: %3; border: none;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::CaptionSize)
        .arg(GlobalStyle::Colors::Border));
    headerLayout->addWidget(m_typeLabel);
    headerLayout->addStretch();
    rootLayout->addWidget(headerWidget);

    // 详情行容器
    m_detailContainer = new QWidget(this);
    m_detailContainer->setObjectName(QStringLiteral("targetDetailRows"));
    auto *detailLayout = new QVBoxLayout(m_detailContainer);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(4);

    auto makeRow = [this](const QString &labelText, QLabel *&valueLabel, const QString &valueObjectName) {
        auto *row = new QWidget(m_detailContainer);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        auto *label = new QLabel(labelText, row);
        // 属性转换：详情行标签交由 labelRole="caption"（TextSecondary/CaptionSize 12px/transparent）；
        // 原 border:none 为 QLabel 默认，完整移除内联样式
        label->setProperty("labelRole", "caption");
        label->setFixedWidth(64);
        rowLayout->addWidget(label);

        valueLabel = new QLabel(row);
        valueLabel->setObjectName(valueObjectName);
        valueLabel->setStyleSheet(QString("color: %1; font-size: %2px; border: none;")
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Fonts::CaptionSize));
        rowLayout->addWidget(valueLabel, 1);
        return row;
    };

    detailLayout->addWidget(makeRow(QStringLiteral("威胁等级"), m_threatValue, QStringLiteral("targetDetailThreatValue")));
    detailLayout->addWidget(makeRow(QStringLiteral("置信度"), m_confValue, QStringLiteral("targetDetailConfValue")));
    detailLayout->addWidget(makeRow(QStringLiteral("坐标"), m_coordValue, QStringLiteral("targetDetailCoordValue")));
    detailLayout->addWidget(makeRow(QStringLiteral("检测设备"), m_deviceValue, QStringLiteral("targetDetailDeviceValue")));
    detailLayout->addWidget(makeRow(QStringLiteral("距跑道"), m_distValue, QStringLiteral("targetDetailDistValue")));

    rootLayout->addWidget(m_detailContainer);

    // 3 操作按钮容器
    m_actionsContainer = new QWidget(this);
    m_actionsContainer->setObjectName(QStringLiteral("targetDetailActions"));
    auto *actionsLayout = new QVBoxLayout(m_actionsContainer);
    actionsLayout->setContentsMargins(0, 4, 0, 0);
    actionsLayout->setSpacing(6);

    const QString primaryBtnStyle = QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %1; "
        "border-radius: 3px; padding: 6px; font-size: %3px; }"
        "QPushButton:hover { background-color: %4; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::CaptionSize)
        .arg(GlobalStyle::Colors::PrimaryGreenHover);
    const QString normalBtnStyle = QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 3px; padding: 6px; font-size: %4px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::PanelBackground)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Fonts::CaptionSize);

    m_createTaskBtn = new QPushButton(QStringLiteral("创建处置任务"), m_actionsContainer);
    m_createTaskBtn->setObjectName(QStringLiteral("targetDetailCreateTaskButton"));
    m_createTaskBtn->setStyleSheet(primaryBtnStyle);
    actionsLayout->addWidget(m_createTaskBtn);

    m_assignDeviceBtn = new QPushButton(QStringLiteral("指派设备"), m_actionsContainer);
    m_assignDeviceBtn->setObjectName(QStringLiteral("targetDetailAssignDeviceButton"));
    m_assignDeviceBtn->setStyleSheet(normalBtnStyle);
    actionsLayout->addWidget(m_assignDeviceBtn);

    m_viewHistoryBtn = new QPushButton(QStringLiteral("查看历史检测"), m_actionsContainer);
    m_viewHistoryBtn->setObjectName(QStringLiteral("targetDetailViewHistoryButton"));
    m_viewHistoryBtn->setStyleSheet(normalBtnStyle);
    actionsLayout->addWidget(m_viewHistoryBtn);

    rootLayout->addWidget(m_actionsContainer);

    // 待检测提示
    m_pendingMsg = new QLabel(this);
    // 属性转换：待检测提示交由 labelRole="caption"（TextSecondary/CaptionSize 12px/transparent）；
    // 原 border:none 为 QLabel 默认，完整移除内联样式
    m_pendingMsg->setProperty("labelRole", "caption");
    m_pendingMsg->setObjectName(QStringLiteral("targetDetailPendingMsg"));
    m_pendingMsg->setWordWrap(true);
    m_pendingMsg->hide();
    rootLayout->addWidget(m_pendingMsg);

    // 操作反馈
    m_feedback = new QLabel(this);
    // 属性转换：操作反馈标签组合 labelRole="caption"（CaptionSize 12px/transparent 排版）
    // + textColor="online"（=StatusOnline，语义色声明在后、优先生效）；
    // 原 border:none 为 QLabel 默认，完整移除内联样式
    m_feedback->setProperty("labelRole", "caption");
    m_feedback->setProperty("textColor", "online");
    m_feedback->setObjectName(QStringLiteral("targetDetailFeedback"));
    m_feedback->setMinimumHeight(16);
    rootLayout->addWidget(m_feedback);

    connect(m_createTaskBtn, &QPushButton::clicked, this, &TargetDetailOverlay::onCreateTaskClicked);
    connect(m_assignDeviceBtn, &QPushButton::clicked, this, &TargetDetailOverlay::onAssignDeviceClicked);
    connect(m_viewHistoryBtn, &QPushButton::clicked, this, &TargetDetailOverlay::onViewHistoryClicked);

    // 关闭按钮定位到右上角（覆盖布局）
    m_closeBtn->raise();
}

void TargetDetailOverlay::showTarget(const Core::TargetInfo &target)
{
    m_target = target;
    m_hasTarget = true;

    m_idLabel->setText(target.id);
    m_typeLabel->setText(target.typeName);

    refreshDetail();
    setFeedback(QString());

    show();
    raise();
}

QString TargetDetailOverlay::currentTargetId() const
{
    return m_hasTarget ? m_target.id : QString();
}

void TargetDetailOverlay::refreshDetail()
{
    // 已检测状态（Detected 及之后）显示详情+操作；否则显示待检测提示
    const bool detected = (m_target.status == Core::TargetStatus::Detected
                           || m_target.status == Core::TargetStatus::Confirmed
                           || m_target.status == Core::TargetStatus::Pending
                           || m_target.status == Core::TargetStatus::Disposing
                           || m_target.status == Core::TargetStatus::Disposed);

    m_evidenceContainer->setVisible(detected);
    m_detailContainer->setVisible(detected);
    m_actionsContainer->setVisible(detected);
    m_pendingMsg->setVisible(!detected);

    if (!detected) {
        m_pendingMsg->setText(QStringLiteral("状态：待检测\n目标尚未检测到，无研判操作（模拟）"));
        return;
    }

    // 威胁等级
    m_threatValue->setText(threatText(m_target.threatLevel));
    m_threatValue->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold; border: none;")
        .arg(threatClass(m_target.threatLevel) == QStringLiteral("high") ? GlobalStyle::Colors::ThreatHigh
             : threatClass(m_target.threatLevel) == QStringLiteral("medium") ? GlobalStyle::Colors::ThreatMedium
             : GlobalStyle::Colors::TextDisabled)
        .arg(GlobalStyle::Fonts::CaptionSize));

    // 置信度
    m_confValue->setText(QString::number(static_cast<int>(m_target.confidence * 100)) + QStringLiteral("%"));

    // 坐标 WGS84 经纬度（position.x()=经度, position.y()=纬度）
    m_coordValue->setText(QStringLiteral("经度:%1° 纬度:%2°")
        .arg(m_target.position.x(), 0, 'f', 6)
        .arg(m_target.position.y(), 0, 'f', 6));

    // 检测设备：TargetInfo 无此字段，显示占位
    m_deviceValue->setText(QStringLiteral("—"));

    // 距跑道：用 position 向量长度模拟
    const double dist = std::sqrt(m_target.position.x() * m_target.position.x()
                                  + m_target.position.y() * m_target.position.y());
    m_distValue->setText(QString::number(static_cast<int>(dist)) + QStringLiteral("m"));
}

QString TargetDetailOverlay::threatText(Core::ThreatLevel level) const
{
    switch (level) {
    case Core::ThreatLevel::Critical: return QStringLiteral("● 严重");
    case Core::ThreatLevel::High:     return QStringLiteral("● 高");
    case Core::ThreatLevel::Medium:   return QStringLiteral("● 中");
    case Core::ThreatLevel::Low:      return QStringLiteral("● 低");
    default:                          return QStringLiteral("● 未知");
    }
}

QString TargetDetailOverlay::threatClass(Core::ThreatLevel level) const
{
    switch (level) {
    case Core::ThreatLevel::Critical:
    case Core::ThreatLevel::High:     return QStringLiteral("high");
    case Core::ThreatLevel::Medium:   return QStringLiteral("medium");
    default:                          return QStringLiteral("unknown");
    }
}

void TargetDetailOverlay::setFeedback(const QString &text)
{
    m_feedback->setText(text);
}

void TargetDetailOverlay::setEvidence(const QImage &annotatedImage, const QDateTime &captureTime,
                                       qint64 videoPositionMs, const QString &provenance)
{
    m_hasEvidence = !annotatedImage.isNull();

    if (m_hasEvidence) {
        // 保持宽高比缩放到视口内尺寸（316x180）
        QPixmap pixmap = QPixmap::fromImage(annotatedImage);
        QPixmap scaled = pixmap.scaled(316, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_evidenceImageLabel->setPixmap(scaled);
        m_evidenceViewport->setAccessibleDescription(
            QStringLiteral("目标 %1 的冻结标注截图").arg(m_hasTarget ? m_target.id : QString()));
    }

    m_captureTimeValue->setText(captureTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    m_videoTimeValue->setText(QTime(0, 0).addMSecs(videoPositionMs).toString(QStringLiteral("mm:ss")));
    m_provenanceValue->setText(provenance);

    refreshEvidence();
}

void TargetDetailOverlay::clearEvidence()
{
    m_hasEvidence = false;
    m_captureTimeValue->setText(QStringLiteral("-"));
    m_videoTimeValue->setText(QStringLiteral("-"));
    m_provenanceValue->setText(QStringLiteral("-"));
    refreshEvidence();
}

void TargetDetailOverlay::reset()
{
    // 重置时必须同时清除目标状态与证据，否则浮层仍持有上一目标的残留数据
    m_target = Core::TargetInfo();
    m_hasTarget = false;
    clearEvidence();
    hide();
}

void TargetDetailOverlay::refreshEvidence()
{
    if (m_hasEvidence) {
        m_evidenceImageLabel->show();
        m_evidencePlaceholder->hide();
    } else {
        m_evidenceImageLabel->clear();
        m_evidenceImageLabel->hide();
        m_evidencePlaceholder->show();
    }
}

void TargetDetailOverlay::onCloseClicked()
{
    m_hasTarget = false;
    hide();
}

void TargetDetailOverlay::onCreateTaskClicked()
{
    const QString taskId = QStringLiteral("TASK-%1").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("HHmmss")));
    setFeedback(QStringLiteral("[模拟] 已创建 %1").arg(taskId));
    emit createTaskRequested(m_target);
}

void TargetDetailOverlay::onAssignDeviceClicked()
{
    setFeedback(QStringLiteral("[模拟] 已发出指派请求，请选择设备"));
    emit assignDeviceRequested(m_target);
}

void TargetDetailOverlay::onViewHistoryClicked()
{
    setFeedback(QStringLiteral("[模拟] 暂无历史检测记录"));
    emit viewHistoryRequested(m_target);
}

void TargetDetailOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 关闭按钮浮动在右上角，留 6px 内边距
    if (m_closeBtn != nullptr) {
        const int margin = 6;
        m_closeBtn->move(width() - m_closeBtn->width() - margin, margin);
    }
}
