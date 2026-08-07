#ifndef MAINWINDOW_MOSGENERATORDIALOG_H
#define MAINWINDOW_MOSGENERATORDIALOG_H

// MOS-015 模拟损毁分布生成器模态：种子化本地随机生成弹坑与 UXO 分布。
// 不联网、不持久化、不写入数据库；JSON 按钮仅发出单向 fixture 导出请求。
// 所有参数均为合成本地 fixture 语义，非真实探测或真实装药。

#include "Core/MOS/MosTypes.h"
#include <QDialog>
#include <QtGlobal>

class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

class MosGeneratorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MosGeneratorDialog(QWidget *parent = nullptr);
    ~MosGeneratorDialog() override;

    // 读取当前表单值
    Core::MOS::MosGeneratorParams currentParams() const;
    qint32 currentSeed() const;

signals:
    // DEC-GEN-APPLY 校验通过后发出，父页面据此生成新障碍物并触发重规划
    void applied(const Core::MOS::MosGeneratorParams &params, qint32 seed);
    // DEC-GEN-JSON 请求父页面通过权威 controller 导出当前已提交 fixture
    void jsonExportRequested(const QString &path);

private:
    void setupUi();
    void revalidate();
    void doExportJson();
    void onApply();

    QSpinBox *m_craterCount{nullptr};
    QDoubleSpinBox *m_craterRMin{nullptr};
    QDoubleSpinBox *m_craterRMax{nullptr};
    QSpinBox *m_uxoCount{nullptr};
    QDoubleSpinBox *m_uxoYMin{nullptr};
    QDoubleSpinBox *m_uxoYMax{nullptr};
    QSpinBox *m_seed{nullptr};
    QLabel *m_banner{nullptr};
    QPushButton *m_applyBtn{nullptr};
};

#endif // MAINWINDOW_MOSGENERATORDIALOG_H
