/**
 * @file MainWindow/TargetDetailPanel.h
 * @brief 目标详情面板
 * @details 显示选中目标的详细信息，包括图片、基本信息、AI推荐等
 * @author 开发团队
 * @date 2024-01-01
 * @version 1.0.0
 */

#ifndef MAINWINDOW_TARGETDETAILPANEL_H
#define MAINWINDOW_TARGETDETAILPANEL_H

#include <QWidget>
#include <QString>
#include "Core/Data/Types.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;

/**
 * @brief 目标详情面板类
 * @details 右侧面板显示目标详情，包含图片轮播、基本信息、AI推荐、操作按钮
 */
class TargetDetailPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit TargetDetailPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TargetDetailPanel();

    /**
     * @brief 设置目标信息
     * @param target 目标信息结构
     */
    void setTarget(const Core::TargetInfo& target);

    /**
     * @brief 清空显示内容
     */
    void clear();

signals:
    /**
     * @brief 创建任务信号
     * @param targetId 目标ID
     */
    void createTaskRequested(const QString& targetId);

    /**
     * @brief 标记误报信号
     * @param targetId 目标ID
     */
    void markAsFalseAlarmRequested(const QString& targetId);

    /**
     * @brief 忽略目标信号
     * @param targetId 目标ID
     */
    void ignoreTargetRequested(const QString& targetId);

private slots:
    /**
     * @brief 创建任务按钮点击
     */
    void onCreateTask();

    /**
     * @brief 标记误报按钮点击
     */
    void onMarkAsFalseAlarm();

    /**
     * @brief 忽略目标按钮点击
     */
    void onIgnoreTarget();

    /**
     * @brief 上一张图片
     */
    void onPrevImage();

    /**
     * @brief 下一张图片
     */
    void onNextImage();

private:
    /**
     * @brief 初始化UI
     */
    void setupUi();

    /**
     * @brief 创建图片轮播区
     * @return QWidget* 图片轮播区指针
     */
    QWidget* createImageCarousel();

    /**
     * @brief 创建基本信息区
     * @return QWidget* 基本信息区指针
     */
    QWidget* createInfoSection();

    /**
     * @brief 创建AI推荐区
     * @return QWidget* AI推荐区指针
     */
    QWidget* createAIRecommendation();

    /**
     * @brief 创建操作按钮区
     * @return QWidget* 操作按钮区指针
     */
    QWidget* createActionButtons();

    /**
     * @brief 更新图片显示
     */
    void updateImageDisplay();

    // 成员变量
    QVBoxLayout *m_mainLayout;           ///< 主布局
    QScrollArea *m_scrollArea;           ///< 滚动区域
    QWidget *m_contentWidget;            ///< 内容widget
    QVBoxLayout *m_contentLayout;        ///< 内容布局

    // 图片轮播
    QLabel *m_imageLabel;                ///< 图片显示标签
    QPushButton *m_prevBtn;              ///< 上一张按钮
    QPushButton *m_nextBtn;              ///< 下一张按钮
    QLabel *m_imageIndicator;             ///< 图片指示器
    int m_currentImageIndex;             ///< 当前图片索引
    int m_totalImages;                   ///< 总图片数

    // 基本信息
    QLabel *m_typeLabel;                 ///< 类型标签
    QLabel *m_modelLabel;                ///< 型号标签
    QLabel *m_confidenceLabel;           ///< 置信度标签
    QLabel *m_positionLabel;              ///< 位置标签
    QLabel *m_depthLabel;                ///< 埋深标签
    QLabel *m_threatLabel;               ///< 威胁等级标签

    // AI推荐
    QLabel *m_aiRecommendationLabel;     ///< AI推荐标签

    // 目标数据
    Core::TargetInfo m_currentTarget;    ///< 当前目标信息
    bool m_hasTarget;                   ///< 是否有目标
};

#endif  // MAINWINDOW_TARGETDETAILPANEL_H
