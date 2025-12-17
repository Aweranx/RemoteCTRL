#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpclient.h"
#include <QMap>
#include <functional>
#include <QTreeWidgetItem>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void testConnect();
    void checkDriverInfo();
    void dealCmd(CPacket& packet);
    void onFileTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpclient;
    QMap<quint16, std::function<void(CPacket& packet)>> m_handler;
    QTreeWidgetItem* m_currentItem;
    void initHandler();
};
#endif // MAINWINDOW_H
