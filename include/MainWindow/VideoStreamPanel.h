#ifndef MAINWINDOW_VIDEOSTREAMPANEL_H
#define MAINWINDOW_VIDEOSTREAMPANEL_H

#include <QWidget>
#include <QList>
#include <QString>

class QLabel;
class QPushButton;
class QGridLayout;
class QStackedWidget;
class QVBoxLayout;

class VideoStreamPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStreamPanel(QWidget *parent = nullptr);
    ~VideoStreamPanel();

    void setStreamCount(int count);
    int streamCount() const;

    void setFullscreenIndex(int index);
    int fullscreenIndex() const;

signals:
    void streamClicked(int index);
    void streamDoubleClicked(int index);
    void fullscreenRequested(int index);

public slots:
    void onLayoutButtonClicked(int count);

private:
    void setupUi();
    void createVideoCells();
    void updateLayout();

    struct VideoCell {
        QWidget *widget;
        QLabel *videoLabel;
        QLabel *nameLabel;
        QLabel *statusLabel;
        QPushButton *fullscreenBtn;
    };

    QStackedWidget *m_stackWidget;
    QWidget *m_gridContainer;
    QGridLayout *m_gridLayout;
    QWidget *m_singleContainer;
    QVBoxLayout *m_singleLayout;

    QList<VideoCell> m_cells;
    int m_streamCount;
    int m_currentLayout;
    int m_fullscreenIndex;

    QWidget *m_controlBar;
    QList<QPushButton*> m_layoutButtons;
    QPushButton *m_fullscreenExitBtn;
};

#endif
