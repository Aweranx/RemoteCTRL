#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QWidget>

class ScreenWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenWidget(QWidget *parent = nullptr);
    // 供外部调用，更新显示的图片
    void updateImage(const QImage& img);

protected:
    // 重写绘图事件
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image; // 保存当前的屏幕截图
signals:
};

#endif // SCREENWIDGET_H
