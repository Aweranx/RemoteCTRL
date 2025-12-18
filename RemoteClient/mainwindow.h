#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tcpclient.h"
#include <QMap>
#include <functional>
#include <QTreeWidgetItem>
#include <QFile>
#include "watchdlg.h"
#include <QTimer>
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
    void watchServer();
    void dealCmd(CPacket& packet);
    void onFileTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onFileListContextMenu(const QPoint &pos);
    void lockMachine();
    void unLockMachine();

private:
    Ui::MainWindow *ui;
    TcpClient* m_tcpclient;
    QMap<quint16, std::function<void(CPacket& packet)>> m_handler;
    QTreeWidgetItem* m_currentItem;
    QFile m_downloadFile;
    bool m_isDownloading;
    qint64 m_receivedSize;
    WatchDlg *m_watchDlg;
    QTimer *timer;
    void initHandler();
};
#endif // MAINWINDOW_H
