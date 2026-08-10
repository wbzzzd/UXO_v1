#ifndef MAINWINDOW_MOSPARAMSPANEL_H
#define MAINWINDOW_MOSPARAMSPANEL_H

// MOS 算法参数栏：10 个可编辑输入 + 2 个只读派生 + 校验横幅 + 规划状态横幅 + 重规划。
// 仅管理本地表单值与校验展示，不持有会话状态、不发起规划、不联网。
// 所有参数均为合成本地 fixture 语义，非真实跑道或工程参数。

#include "Core/MOS/MosTypes.h"
#include "Core/MOS/MosValidation.h"
#include <QWidget>

class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;

class MosParamsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MosParamsPanel(QWidget *parent = nullptr);
    ~MosParamsPanel() override;

    // 读取当前表单值（合成参数）
    Core::MOS::MosRunwayParams currentParams() const;
    // 用参数值回填表单
    void setParams(const Core::MOS::MosRunwayParams &params);
    // 更新只读派生字段（损毁点总数 / 模拟处理假设数）
    void setDerivedCounts(int damageCount, int repairedCount);

    // NoFeasible：合法输入但所有档位均无可行矩形（合法无解），区别于 Error 的拒绝
    enum class PlanState { Idle, Planning, Loading, Result, Error, Empty, NoFeasible };
    // 更新规划状态横幅 DEC-CE-PLAN-STATE
    void setPlanState(PlanState state, int tierCount);

    // 当前表单是否通过校验（决定 DEC-CE-PARAM-REPLAN 是否可用）
    bool isValid() const;

signals:
    // 表单参数变更时发出（valid 表示是否通过校验）
    void paramsEdited(bool valid);
    // DEC-CE-PARAM-REPLAN 点击
    void replanRequested();
    // DEC-CE-PARAM-RESET 点击
    void resetRequested();

private:
    void setupUi();
    void revalidate();

    QDoubleSpinBox *m_length{nullptr};
    QDoubleSpinBox *m_width{nullptr};
    QDoubleSpinBox *m_minLength{nullptr};
    QDoubleSpinBox *m_minWidth{nullptr};
    QDoubleSpinBox *m_k{nullptr};
    QDoubleSpinBox *m_step{nullptr};
    QDoubleSpinBox *m_backfill{nullptr};
    QDoubleSpinBox *m_uxoHours{nullptr};
    QDoubleSpinBox *m_expand{nullptr};
    QSpinBox *m_tiers{nullptr};
    QLabel *m_dmgCount{nullptr};
    QLabel *m_repairedCount{nullptr};
    QLabel *m_validationBanner{nullptr};
    QLabel *m_planStateBanner{nullptr};
    QPushButton *m_resetBtn{nullptr};
    QPushButton *m_replanBtn{nullptr};
    bool m_valid{true};
};

#endif // MAINWINDOW_MOSPARAMSPANEL_H
