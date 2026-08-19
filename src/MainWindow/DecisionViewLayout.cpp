// DecisionView 视口缩放实现：从 DecisionView.cpp 拆分以满足单文件纯代码行门禁。
// 缩放策略：clamp(min(w/1920, h/1080), 1.0, 2.0)，不乘 devicePixelRatio。

#include "MainWindow/DecisionView.h"
#include "MainWindow/MosGeneratorDialog.h"
#include "MainWindow/MosRunwayWidget.h"
#include "Common/GlobalStyle.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStyle>
#include <QString>
#include <QWidget>

#include <algorithm>

double DecisionView::viewportScale() const
{
    // 按当前尺寸即时计算缩放系数：测试可在不 show 的情况下断言缩放策略
    const double ratioW = static_cast<double>(width()) / GlobalStyle::Sizes::DecisionReferenceWidth;
    const double ratioH = static_cast<double>(height()) / GlobalStyle::Sizes::DecisionReferenceHeight;
    return std::clamp(std::min(ratioW, ratioH),
                      GlobalStyle::Sizes::DecisionViewportScaleMin,
                      GlobalStyle::Sizes::DecisionViewportScaleMax);
}

void DecisionView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyViewportScale();
}

void DecisionView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 页面被 QStackedWidget 隐藏期间若发生尺寸变更，resizeEvent 已按隐藏态几何
    // 计算（可能为 0/1x）且幂等守卫会跳过后续相同 scale 的重算；页面变可见时
    // 显式重跑 applyViewportScale，确保 4K 等高 DPI 场景下缩放策略实际生效。
    applyViewportScale();
}

void DecisionView::applyViewportScale()
{
    // 视口缩放策略：clamp(min(w/1920, h/1080), 1.0, 2.0)
    // 1280x720 与 1920x1080 均为 1.0；3840x2160 为 2.0；不乘 devicePixelRatio
    const int w = width();
    const int h = height();
    const double ratioW = static_cast<double>(w) / GlobalStyle::Sizes::DecisionReferenceWidth;
    const double ratioH = static_cast<double>(h) / GlobalStyle::Sizes::DecisionReferenceHeight;
    const double raw = std::min(ratioW, ratioH);
    const double scale = std::clamp(raw,
                                    GlobalStyle::Sizes::DecisionViewportScaleMin,
                                    GlobalStyle::Sizes::DecisionViewportScaleMax);
    if (scale == m_viewportScale) {
        // 缩放未变：跳过样式重算，避免重复 resize 触发 QSS 重解析
        return;
    }
    m_viewportScale = scale;

    // 工具栏高度与左右面板宽度按 scale 线性缩放
    m_toolbar->setFixedHeight(static_cast<int>(GlobalStyle::Sizes::DecisionToolbarBaseHeight * scale));
    m_leftPanel->setFixedWidth(static_cast<int>(GlobalStyle::Sizes::DecisionLeftPanelBaseWidth * scale));
    m_rightPanel->setFixedWidth(static_cast<int>(GlobalStyle::Sizes::DecisionRightPanelBaseWidth * scale));

    // 子控件字体缩放：用动态属性 + QSS 选择器，避免对每个控件 setFont
    // 每次重设属性值会触发该控件 QSS 重评估；首帧 scale=1.0 时也写入，确保默认态一致
    const int scaledBodyPt = static_cast<int>(GlobalStyle::Fonts::BodySize * scale);
    const int scaledTitlePt = static_cast<int>(GlobalStyle::Fonts::TitleSize * scale);
    const int scaledCaptionPt = static_cast<int>(GlobalStyle::Fonts::CaptionSize * scale);
    // field 角色为 MOS 参数标签专用：1x 基准 11px（比 caption 更紧凑），
    // 沿用 1x/2x 缩放策略，2x（4K）下为 22px，避免 1280 下单位文本被换行截断。
    const int scaledFieldPt = static_cast<int>(11 * scale);
    // 警示：控件级样式表禁止携带裸 background-color 声明——它会连带失效应用级
    // 样式表对本控件整棵子树的 containerBg 属性规则，子容器全部回退为本体底色
    // （minirepro5 裸树五阶段实验实证：仅裸声明即致命，仅字体规则无害）。
    // 本体底色由构造函数的 containerBg="main" 属性 + GlobalStyle 规则负责，
    // 本样式表只承载 mosFontRole 字号缩放。
    const QString scaledStyle = QStringLiteral(
        "QWidget[mosFontRole=\"body\"] { font-size: %1px; }"
        "QWidget[mosFontRole=\"title\"] { font-size: %2px; }"
        "QWidget[mosFontRole=\"caption\"] { font-size: %3px; }"
        "QWidget[mosFontRole=\"field\"] { font-size: %4px; }"
    ).arg(scaledBodyPt).arg(scaledTitlePt).arg(scaledCaptionPt).arg(scaledFieldPt);
    // 字体缩放注入总开关（当前关闭，所有 scale 均注入空样式表）：这组 mosFontRole
    // 字号规则自批次1-5 引入以来从未在任何基线生效过——裸声明事故使其在全部
    // scale 下连带失效，1x 与 4K 基线一直按应用级默认字号渲染并通过门禁。移除
    // 裸声明后若开启注入：1x 下整页标签将从默认字号抬到 body=14px（standalone
    // 实测增高 46px、行距逐行下移）；4K 下产生 14.28% 布局漂移（3840x2160 矩阵
    // 实测）。是否让字号随视口放大属可见视觉变更，待用户单独裁决后改为 true；
    // 本批次目标为像素对齐迁移，保持关闭。
    constexpr bool injectViewportFontScale = false;
    // 用 setProperty 触发样式刷新：动态属性改变后需显式 unpolish/polish 才生效
    setStyleSheet(injectViewportFontScale ? scaledStyle : QString());
    // 中心跑道画布按视口缩放重设内部字体/笔宽
    m_runway->setViewportScale(scale);

    // 步进框基础高度按 scale 缩放：4K 下 28px 字号 + padding 不再被 26px 裁剪，
    // 故基础高度提到 30；横幅仍维持 24 基础不变
    const int scaledSpinBoxHeight = static_cast<int>(30 * scale);
    const int scaledBannerHeight = static_cast<int>(24 * scale);

    // 给所有相关子控件打上动态属性角色：覆盖 QLabel/QPushButton/QAbstractSpinBox/
    // QAbstractItemView（含 QListWidget 与表视图），避免 4K 下仅少数被显式赋值的控件
    // 随缩放，其余停在 14px 默认态。已显式赋值 caption/title/field 等角色的控件保持原角色，
    // 仅对未设角色的控件默认 "body"；标题/说明标签在循环后覆盖。
    for (QWidget *child : findChildren<QWidget *>()) {
        if (child->property("mosFontRole").isValid()) {
            continue;
        }
        if (auto *sb = qobject_cast<QAbstractSpinBox *>(child)) {
            child->setProperty("mosFontRole", QLatin1String("body"));
            sb->setFixedHeight(scaledSpinBoxHeight);
        } else if (qobject_cast<QAbstractItemView *>(child)) {
            child->setProperty("mosFontRole", QLatin1String("body"));
        } else if (qobject_cast<QLabel *>(child) || qobject_cast<QPushButton *>(child)) {
            child->setProperty("mosFontRole", QLatin1String("body"));
        }
    }

    // 特定角色覆盖：跑道标题与档位标题用 title，缩放显示用 caption
    auto assignRole = [](QWidget *w, const char *role){
        if (w) w->setProperty("mosFontRole", QLatin1String(role));
    };
    assignRole(m_rwTitle, "title");
    assignRole(m_zoomLevel, "caption");
    assignRole(m_detailTier, "title");

    // 校验/规划状态横幅按 scale 重设高度（MosParamsPanel::setupUi 初始为 24）
    if (auto *validationBanner = findChild<QLabel *>(QStringLiteral("DEC-CE-VALIDATION"))) {
        validationBanner->setFixedHeight(scaledBannerHeight);
    }
    if (auto *planStateBanner = findChild<QLabel *>(QStringLiteral("DEC-CE-PLAN-STATE"))) {
        planStateBanner->setFixedHeight(scaledBannerHeight);
    }

    // 模态生成器对话框按视口缩放：参考尺寸 520x360 × scale，并设置缩放后最小尺寸；
    // 关闭按钮/校验横幅的固定高度同步缩放；步进框高度已由上方通用
    // QAbstractSpinBox 循环按 30*scale 缩放，此处不重复处理
    if (m_generatorDialog) {
        const int dialogW = static_cast<int>(520 * scale);
        const int dialogH = static_cast<int>(360 * scale);
        m_generatorDialog->setMinimumSize(dialogW, dialogH);
        m_generatorDialog->resize(dialogW, dialogH);
        // DEC-GEN-CLOSE 关闭按钮：基础 28x28 × scale
        if (auto *genClose = m_generatorDialog->findChild<QPushButton *>(QStringLiteral("DEC-GEN-CLOSE"))) {
            const int closeSize = static_cast<int>(28 * scale);
            genClose->setFixedSize(closeSize, closeSize);
        }
        // DEC-GEN-BANNER 校验横幅：基础高度 28 × scale（与 DEC-CE-* 横幅策略一致）
        if (auto *genBanner = m_generatorDialog->findChild<QLabel *>(QStringLiteral("DEC-GEN-BANNER"))) {
            genBanner->setFixedHeight(static_cast<int>(28 * scale));
        }
    }

    // 触发已存在控件的样式刷新：动态属性改变后需显式 unpolish/polish 才生效
    for (QWidget *child : findChildren<QWidget *>()) {
        if (child->property("mosFontRole").isValid()) {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }
    }
}
