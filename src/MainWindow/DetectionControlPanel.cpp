#include "MainWindow/DetectionControlPanel.h"
#include "Common/GlobalStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>
#include <QDateTime>
#include <QScrollBar>

DetectionControlPanel::DetectionControlPanel(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_takeoffBtn(nullptr)
    , m_landBtn(nullptr)
    , m_returnBtn(nullptr)
    , m_altitudeSlider(nullptr)
    , m_altitudeLabel(nullptr)
    , m_logEdit(nullptr)
{
    setupUi();
}

DetectionControlPanel::~DetectionControlPanel()
{
}

void DetectionControlPanel::setupUi()
{
    setStyleSheet(QString("background-color: %1;").arg(GlobalStyle::Colors::PanelBackground));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    QWidget *header = new QWidget(this);
    header->setFixedHeight(28);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 0, 4, 0);

    m_titleLabel = new QLabel("探测控制", header);
    m_titleLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: bold;")
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Fonts::BodySize));
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    QWidget *controlRow = new QWidget(this);
    controlRow->setFixedHeight(36);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlRow);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(6);

    QString btnStyle = QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::PrimaryGreen)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::PrimaryGreenHover);

    QString dangerBtnStyle = QString(
        "QPushButton { background-color: %1; color: %2; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; }"
        "QPushButton:hover { background-color: %3; }")
        .arg(GlobalStyle::Colors::DangerRed)
        .arg(GlobalStyle::Colors::TextPrimary)
        .arg(GlobalStyle::Colors::DangerRedHover);

    m_takeoffBtn = new QPushButton("起飞", controlRow);
    m_takeoffBtn->setStyleSheet(btnStyle);
    connect(m_takeoffBtn, &QPushButton::clicked, this, &DetectionControlPanel::takeoffRequested);
    controlLayout->addWidget(m_takeoffBtn);

    m_landBtn = new QPushButton("降落", controlRow);
    m_landBtn->setStyleSheet(btnStyle);
    connect(m_landBtn, &QPushButton::clicked, this, &DetectionControlPanel::landRequested);
    controlLayout->addWidget(m_landBtn);

    m_returnBtn = new QPushButton("返航", controlRow);
    m_returnBtn->setStyleSheet(dangerBtnStyle);
    connect(m_returnBtn, &QPushButton::clicked, this, &DetectionControlPanel::returnToBaseRequested);
    controlLayout->addWidget(m_returnBtn);

    controlLayout->addStretch();

    QLabel *altTitle = new QLabel("高度:", controlRow);
    altTitle->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextSecondary));
    controlLayout->addWidget(altTitle);

    m_altitudeSlider = new QSlider(Qt::Horizontal, controlRow);
    m_altitudeSlider->setRange(10, 200);
    m_altitudeSlider->setValue(50);
    m_altitudeSlider->setFixedWidth(80);
    m_altitudeSlider->setStyleSheet(QString(
        "QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; width: 12px; height: 12px; margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; }")
        .arg(GlobalStyle::Colors::Border)
        .arg(GlobalStyle::Colors::PrimaryGreen));
    connect(m_altitudeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_altitudeLabel->setText(QString("%1m").arg(value));
        emit altitudeChanged(value);
    });
    controlLayout->addWidget(m_altitudeSlider);

    m_altitudeLabel = new QLabel("50m", controlRow);
    m_altitudeLabel->setFixedWidth(36);
    m_altitudeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(GlobalStyle::Colors::TextPrimary));
    controlLayout->addWidget(m_altitudeLabel);

    mainLayout->addWidget(controlRow);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    m_logEdit->setStyleSheet(QString(
        "QTextEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 11px; padding: 4px; }")
        .arg(GlobalStyle::Colors::Background)
        .arg(GlobalStyle::Colors::TextSecondary)
        .arg(GlobalStyle::Colors::Border));

    m_logEdit->setText(
        QString("[%1] 系统就绪，等待指令\n").arg(QDateTime::currentDateTime().toString("HH:mm:ss")));

    mainLayout->addWidget(m_logEdit);
}

void DetectionControlPanel::addLogEntry(const QString& message)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(timeStr).arg(message));
    QScrollBar *scrollBar = m_logEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void DetectionControlPanel::clearLog()
{
    m_logEdit->clear();
}
