#include "MainWindow/VideoStreamPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QDateTime>

VideoStreamPanel::VideoStreamPanel(QWidget *parent)
    : QWidget(parent)
    , m_stackWidget(nullptr)
    , m_gridContainer(nullptr)
    , m_gridLayout(nullptr)
    , m_singleContainer(nullptr)
    , m_singleLayout(nullptr)
    , m_streamCount(4)
    , m_currentLayout(4)
    , m_fullscreenIndex(-1)
    , m_controlBar(nullptr)
    , m_fullscreenExitBtn(nullptr)
    , m_updateTimer(nullptr)
{
    setupUi();
}

VideoStreamPanel::~VideoStreamPanel()
{
}

void VideoStreamPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::Background));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_stackWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackWidget, 1);

    m_gridContainer = new QWidget(this);
    m_gridContainer->setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::Background));
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(2, 2, 2, 2);
    m_gridLayout->setSpacing(2);

    m_singleContainer = new QWidget(this);
    m_singleContainer->setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::Background));
    m_singleLayout = new QVBoxLayout(m_singleContainer);
    m_singleLayout->setContentsMargins(2, 2, 2, 2);
    m_singleLayout->setSpacing(0);

    m_fullscreenExitBtn = new QPushButton("退出全屏", m_singleContainer);
    m_fullscreenExitBtn->setFixedSize(80, 28);
    m_fullscreenExitBtn->setStyleSheet(QString(
        "QPushButton { background-color: rgba(0,0,0,150); color: %1; border: none; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: rgba(0,0,0,200); }")
        .arg(GlobalStyle::Colors::TextPrimary));
    m_fullscreenExitBtn->hide();
    connect(m_fullscreenExitBtn, &QPushButton::clicked, this, [this]() {
        setFullscreenIndex(-1);
    });

    m_stackWidget->addWidget(m_gridContainer);
    m_stackWidget->addWidget(m_singleContainer);

    createVideoCells();
    updateLayout();

    m_controlBar = new QWidget(this);
    m_controlBar->setFixedHeight(32);
    m_controlBar->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;")
        .arg(GlobalStyle::Colors::ToolbarBackground)
        .arg(GlobalStyle::Colors::Border));

    QHBoxLayout *controlLayout = new QHBoxLayout(m_controlBar);
    controlLayout->setContentsMargins(8, 2, 8, 2);
    controlLayout->setSpacing(4);

    QLabel *layoutLabel = new QLabel("分屏:", m_controlBar);
    layoutLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    controlLayout->addWidget(layoutLabel);

    for (int count : {1, 2, 3, 4}) {
        QPushButton *btn = new QPushButton(QString::number(count), m_controlBar);
        btn->setFixedSize(28, 24);
        btn->setProperty("layoutCount", count);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; font-size: 12px; }"
            "QPushButton:hover { background-color: %3; }"
            "QPushButton[active=\"true\"] { background-color: %4; color: %2; border: 1px solid %4; }")
            .arg(GlobalStyle::Colors::Background)
            .arg(GlobalStyle::Colors::TextPrimary)
            .arg(GlobalStyle::Colors::Border)
            .arg(GlobalStyle::Colors::PrimaryGreen));
        if (count == m_currentLayout) {
            btn->setProperty("active", true);
        }
        connect(btn, &QPushButton::clicked, this, [this, count]() {
            onLayoutButtonClicked(count);
        });
        controlLayout->addWidget(btn);
        m_layoutButtons.append(btn);
    }

    controlLayout->addStretch();

    QPushButton *fullscreenBtn = new QPushButton("全屏", m_controlBar);
    fullscreenBtn->setFixedSize(48, 24);
    fullscreenBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::Border));
    connect(fullscreenBtn, &QPushButton::clicked, this, [this]() {
        if (m_fullscreenIndex < 0 && !m_cells.isEmpty()) {
            setFullscreenIndex(0);
        }
    });
    controlLayout->addWidget(fullscreenBtn);

    mainLayout->addWidget(m_controlBar);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &VideoStreamPanel::updateCellContents);
    m_updateTimer->start(1000);
}

void VideoStreamPanel::createVideoCells()
{
    QStringList streamNames = {"UAV-1 侦察无人机", "UAV-2 排爆无人机", "Robot-1 排爆机器人", "GPR-1 探地雷达"};
    QStringList streamStatuses = {"在线 | 信号: 95%", "在线 | 信号: 88%", "在线 | 信号: 92%", "离线"};

    for (int i = 0; i < m_streamCount; ++i) {
        VideoCell cell;

        cell.widget = new QWidget(m_gridContainer);
        cell.widget->setStyleSheet(QString(
            "background-color: %1; border: 1px solid %2; border-radius: 2px;")
            .arg("#1A1A1A")
            .arg(GlobalStyle::Colors::Border));

        QVBoxLayout *cellLayout = new QVBoxLayout(cell.widget);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(0);

        cell.videoLabel = new QLabel(cell.widget);
        cell.videoLabel->setAlignment(Qt::AlignCenter);
        cell.videoLabel->setMinimumHeight(60);
        cell.videoLabel->setStyleSheet("background-color: #0D0D0D; color: #666; font-size: 24px;");
        cell.videoLabel->setText("●");
        cellLayout->addWidget(cell.videoLabel, 1);

        QWidget *infoBar = new QWidget(cell.widget);
        infoBar->setFixedHeight(28);
        infoBar->setStyleSheet(QString("background-color: rgba(0,0,0,180);"));

        QHBoxLayout *infoLayout = new QHBoxLayout(infoBar);
        infoLayout->setContentsMargins(8, 2, 8, 2);

        QString name = (i < streamNames.size()) ? streamNames[i] : QString("视频 %1").arg(i + 1);
        cell.nameLabel = new QLabel(name, infoBar);
        cell.nameLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;")
            .arg(GlobalStyle::Colors::TextPrimary));
        infoLayout->addWidget(cell.nameLabel);

        infoLayout->addStretch();

        QString status = (i < streamStatuses.size()) ? streamStatuses[i] : "未知";
        cell.statusLabel = new QLabel(status, infoBar);
        QString statusColor = status.contains("离线") ? GlobalStyle::Colors::TextDisabled : GlobalStyle::Colors::StatusOnline;
        cell.statusLabel->setStyleSheet(QString("color: %1; font-size: 10px;").arg(statusColor));
        infoLayout->addWidget(cell.statusLabel);

        cell.fullscreenBtn = new QPushButton("⛶", infoBar);
        cell.fullscreenBtn->setFixedSize(20, 20);
        cell.fullscreenBtn->setStyleSheet(
            "QPushButton { background: transparent; color: #AAA; border: none; font-size: 14px; }"
            "QPushButton:hover { color: white; }");
        connect(cell.fullscreenBtn, &QPushButton::clicked, this, [this, i]() {
            setFullscreenIndex(i);
        });
        infoLayout->addWidget(cell.fullscreenBtn);

        cellLayout->addWidget(infoBar);

        m_cells.append(cell);
    }
}

void VideoStreamPanel::updateLayout()
{
    while (m_gridLayout->count() > 0) {
        QLayoutItem *item = m_gridLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
        }
        delete item;
    }

    int visibleCount = qMin(m_currentLayout, m_streamCount);

    if (visibleCount == 1) {
        m_gridLayout->addWidget(m_cells[0].widget, 0, 0);
        m_cells[0].widget->show();
        for (int i = 1; i < m_cells.size(); ++i) {
            m_cells[i].widget->hide();
        }
    } else if (visibleCount == 2) {
        m_gridLayout->addWidget(m_cells[0].widget, 0, 0);
        m_gridLayout->addWidget(m_cells[1].widget, 0, 1);
        for (int i = 0; i < 2; ++i) m_cells[i].widget->show();
        for (int i = 2; i < m_cells.size(); ++i) m_cells[i].widget->hide();
    } else if (visibleCount == 3) {
        m_gridLayout->addWidget(m_cells[0].widget, 0, 0, 1, 2);
        m_gridLayout->addWidget(m_cells[1].widget, 1, 0);
        m_gridLayout->addWidget(m_cells[2].widget, 1, 1);
        for (int i = 0; i < 3; ++i) m_cells[i].widget->show();
        for (int i = 3; i < m_cells.size(); ++i) m_cells[i].widget->hide();
    } else {
        m_gridLayout->addWidget(m_cells[0].widget, 0, 0);
        m_gridLayout->addWidget(m_cells[1].widget, 0, 1);
        m_gridLayout->addWidget(m_cells[2].widget, 1, 0);
        m_gridLayout->addWidget(m_cells[3].widget, 1, 1);
        for (int i = 0; i < 4 && i < m_cells.size(); ++i) m_cells[i].widget->show();
        for (int i = 4; i < m_cells.size(); ++i) m_cells[i].widget->hide();
    }

    for (QPushButton *btn : m_layoutButtons) {
        bool active = btn->property("layoutCount").toInt() == m_currentLayout;
        btn->setProperty("active", active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void VideoStreamPanel::updateCellContents()
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    for (int i = 0; i < m_cells.size(); ++i) {
        if (m_cells[i].widget->isVisible()) {
            m_cells[i].videoLabel->setText(
                QString("● REC %1\n%2").arg(i + 1).arg(timeStr));
        }
    }
}

void VideoStreamPanel::setStreamCount(int count)
{
    m_streamCount = count;
    updateLayout();
}

int VideoStreamPanel::streamCount() const
{
    return m_streamCount;
}

void VideoStreamPanel::setFullscreenIndex(int index)
{
    m_fullscreenIndex = index;

    if (index >= 0 && index < m_cells.size()) {
        m_cells[index].widget->setParent(m_singleContainer);
        m_singleLayout->insertWidget(0, m_cells[index].widget, 1);
        m_cells[index].widget->show();
        m_fullscreenExitBtn->raise();
        m_fullscreenExitBtn->show();
        m_stackWidget->setCurrentIndex(1);
        m_controlBar->hide();
        emit fullscreenRequested(index);
    } else {
        m_fullscreenIndex = -1;
        for (int i = 0; i < m_cells.size(); ++i) {
            m_cells[i].widget->setParent(m_gridContainer);
        }
        m_fullscreenExitBtn->hide();
        m_stackWidget->setCurrentIndex(0);
        m_controlBar->show();
        updateLayout();
    }
}

int VideoStreamPanel::fullscreenIndex() const
{
    return m_fullscreenIndex;
}

void VideoStreamPanel::onLayoutButtonClicked(int count)
{
    m_currentLayout = count;
    if (m_fullscreenIndex >= 0) {
        setFullscreenIndex(-1);
    }
    updateLayout();
}
