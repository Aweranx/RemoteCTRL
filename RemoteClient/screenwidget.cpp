#include "screenwidget.h"
#include <QPainter>

ScreenWidget::ScreenWidget(QWidget *parent) : QWidget(parent)
{
    // 设置背景为黑色
    setAttribute(Qt::WA_OpaquePaintEvent);
    QPalette pal(this->palette());
    pal.setColor(QPalette::Window, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(pal);
}

void ScreenWidget::updateImage(const QImage &img)
{
    m_image = img;
    // 调用paintEvent重新绘制屏幕截图
    this->update();
}

void ScreenWidget::paintEvent(QPaintEvent *event)
{
    if (m_image.isNull()) return;

    QPainter painter(this);
    // 开启抗锯齿（平滑缩放）
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    // 目标矩形：保持纵横比居中显示
    QRect rect = this->rect();
    // 将图片按比例画在窗口中间
    QImage scaledImg = m_image.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // 计算居中位置
    int x = (rect.width() - scaledImg.width()) / 2;
    int y = (rect.height() - scaledImg.height()) / 2;

    painter.drawImage(x, y, scaledImg);
}

// 将 Qt鼠标事件转换为协议结构体
MOUSEEV ScreenWidget::createMouseEvent(QMouseEvent *event, int action)
{
    MOUSEEV mouse;

    // 坐标映射 (本地坐标 -> 远程坐标)
    // 公式：远程X = 本地X * (远程宽 / 本地宽)
    if (width() > 0 && height() > 0) {
        double xRatio = (double)m_remoteWidth / width();
        double yRatio = (double)m_remoteHeight / height();
        mouse.ptXY.x = event->pos().x() * xRatio;
        mouse.ptXY.y = event->pos().y() * yRatio;
    } else {
        mouse.ptXY.x = 0; mouse.ptXY.y = 0;
    }

    if (event->button() == Qt::LeftButton)       mouse.nButton = 0; // 左
    else if (event->button() == Qt::RightButton) mouse.nButton = 1; // 右
    else if (event->button() == Qt::MiddleButton) mouse.nButton = 2; // 中
    else mouse.nButton = 4; // 无按键 (移动时)

    mouse.nAction = action;

    return mouse;
}

void ScreenWidget::mousePressEvent(QMouseEvent *event) {
    // Action: 2 (按下)
    emit sigMouseEvent(createMouseEvent(event, 2));
}

void ScreenWidget::mouseReleaseEvent(QMouseEvent *event) {
    // Action: 3 (放开)
    emit sigMouseEvent(createMouseEvent(event, 3));
}

void ScreenWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    // Action: 1 (双击)
    emit sigMouseEvent(createMouseEvent(event, 1));
}
