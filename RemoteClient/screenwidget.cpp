#include "screenwidget.h"
#include <QPainter>

ScreenWidget::ScreenWidget(QWidget *parent) : QWidget(parent)
{
    // 设置背景为黑色 (防止图片比例不符时留白太难看)
    setAttribute(Qt::WA_OpaquePaintEvent); // 优化性能，不画背景
    QPalette pal(this->palette());
    pal.setColor(QPalette::Window, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(pal);
}

void ScreenWidget::updateImage(const QImage &img)
{
    m_image = img;
    // 触发重绘，会调用 paintEvent
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

    // 核心逻辑：将图片按比例画在窗口中间
    QImage scaledImg = m_image.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 计算居中位置
    int x = (rect.width() - scaledImg.width()) / 2;
    int y = (rect.height() - scaledImg.height()) / 2;

    painter.drawImage(x, y, scaledImg);
}
