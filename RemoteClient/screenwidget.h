#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QWidget>
#include "tcpclient.h"
#include <QMouseEvent>

class ScreenWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenWidget(QWidget *parent = nullptr);
    // 供外部调用，更新显示的图片
    void updateImage(const QImage& img);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
signals:
    void sigMouseEvent(const MOUSEEV& ev);
private:
    MOUSEEV createMouseEvent(QMouseEvent *event, int action);
    QImage m_image; // 保存当前的屏幕截图
    int m_remoteWidth = 2560;
    int m_remoteHeight = 1440;
};

#endif // SCREENWIDGET_H
