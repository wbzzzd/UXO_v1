#ifndef DECISIONVIEW_H
#define DECISIONVIEW_H

// 决策页：MOS 起降带规划工作区（P0 Approved）。
// 三栏布局：左损毁目标列表 + 中心跑道俯视图与算法参数 + 右候选方案与当前模拟选择摘要。
// 仅按传入的 MosPlanningSnapshot 副本渲染，被动发出信号；不持有会话状态、不发起规划、不联网。
// 所有数据均为本地合成 fixture 语义，非真实跑道、真实弹坑或真实安全结论。

#include "Core/MOS/MosPlanningSession.h"
#include "Core/MOS/MosTypes.h"
#include <QString>
#include <QWidget>

class MosRunwayWidget;
class MosParamsPanel;
class MosGeneratorDialog;
class TargetCardWidget;
class PlanCardWidget;
class QListWidget;
class QLabel;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QScrollBar;
class QSplitter;
class QVBoxLayout;

namespace Core::MOS { class MosPlanningController; }

class DecisionView : public QWidget
{
    Q_OBJECT
public:
    explicit DecisionView(QWidget *parent = nullptr);
    ~DecisionView() override;

    // 用快照副本刷新整页（被动入口，由外部控制器调用）
    void setSnapshot(const Core::MOS::MosPlanningSnapshot &snapshot);
    // 进入/退出规划中状态（控制工具栏与状态栏）
    void setPlanning(bool planning);
    // 切换档位选中态（被动入口，仅刷新视觉，不重建控件）
    void selectTier(int tierIndex);

    // 读取当前参数栏表单值（合成参数）
    Core::MOS::MosRunwayParams currentParams() const;
    // 读取当前快照中的障碍物集合
    Core::MOS::MosObstacleSet currentObstacles() const;
    // 读取当前视口缩放系数（用于测试断言）
    double viewportScale() const;

    // 注入权威 controller，转发到生成器模态供 JSON 按钮直接调用
    void setMosController(Core::MOS::MosPlanningController *controller);

signals:
    // DEC-TB-GEN 点击：请求打开生成器模态
    void generatorRequested();
    // DEC-CE-PARAM-REPLAN 点击
    void replanRequested();
    // 档位切换（DEC-TB-PLAN-N / 卡片 / 跑道矩形）
    void tierSelected(int tierIndex);
    // 目标选择（左面板卡片 / 跑道圆圈）
    void targetSelected(const QString &targetId);
    // 生成器应用（DEC-GEN-APPLY 校验通过后）
    void generatorApplied(const Core::MOS::MosGeneratorParams &params, qint32 seed);

protected:
    void resizeEvent(QResizeEvent *event) override;
    // 页面在 QStackedWidget 中首次或再次变可见时（如 DEC-NAV-03 切入决策页），
    // 此前 resizeEvent 可能以 0/1x 尺寸运行而未应用 4K 缩放；showEvent 在变可见时
    // 重跑 applyViewportScale，确保缩放策略在页面可见时必定生效。
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void rebuildTierButtons(int tierCount);
    void selectTarget(const QString &targetId);
    void openGenerator();
    // 按当前尺寸重算视口缩放并应用到工具栏/左右面板/跑道字体
    // 缩放策略：clamp(min(w/1920, h/1080), 1.0, 2.0)，不乘 devicePixelRatio
    void applyViewportScale();

    // 顶部 MOS 工具栏
    QWidget *m_toolbar{nullptr};
    QPushButton *m_tbGen{nullptr};
    QPushButton *m_tbParams{nullptr};
    QWidget *m_tierButtonContainer{nullptr};
    QVector<QPushButton *> m_tierButtons;
    QVector<PlanCardWidget *> m_planCards;

    // 左面板
    QWidget *m_leftPanel{nullptr};
    QListWidget *m_targetList{nullptr};

    // 中心区
    QWidget *m_centerPanel{nullptr};
    QLabel *m_rwTitle{nullptr};
    MosRunwayWidget *m_runway{nullptr};
    QScrollBar *m_hScrollBar{nullptr};
    QPushButton *m_zoomIn{nullptr};
    QPushButton *m_zoomOut{nullptr};
    QPushButton *m_zoomReset{nullptr};
    QLabel *m_zoomLevel{nullptr};
    MosParamsPanel *m_paramsPanel{nullptr};

    // 右面板
    QWidget *m_rightPanel{nullptr};
    QWidget *m_plansContainer{nullptr};
    QVBoxLayout *m_plansLayout{nullptr};
    QLabel *m_detailArea{nullptr};
    QLabel *m_detailTier{nullptr};
    QLabel *m_detailSize{nullptr};
    QLabel *m_detailArea2{nullptr};
    QLabel *m_detailHours{nullptr};
    QLabel *m_detailDamage{nullptr};
    QWidget *m_p1Slot{nullptr};

    // 状态栏
    QLabel *m_sbDevice{nullptr};
    QLabel *m_sbSim{nullptr};
    QLabel *m_sbAlarm{nullptr};
    QLabel *m_sbTarget{nullptr};

    MosGeneratorDialog *m_generatorDialog{nullptr};
    Core::MOS::MosPlanningSnapshot m_snapshot;
    bool m_planning{false};
    // 当前选中的目标标识：跨快照刷新时复用，避免被列表重建清空
    QString m_selectedTargetId;
    // 当前视口缩放系数（applyViewportScale 写入，viewportScale 读取）
    // 初始化为 -1.0 哨兵，确保首次 applyViewportScale 必定执行
    double m_viewportScale{-1.0};
    // 三栏 splitter：resize 时按缩放重设左右面板宽度
    QSplitter *m_splitter{nullptr};
};

#endif // DECISIONVIEW_H
