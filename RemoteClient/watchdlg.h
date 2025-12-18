#ifndef WATCHDLG_H
#define WATCHDLG_H

#include <QMainWindow>
#include <QMouseEvent>
#include "tcpclient.h"

namespace Ui {
class WatchDlg;
}

class WatchDlg : public QMainWindow
{
    Q_OBJECT

public:
    explicit WatchDlg(QWidget *parent = nullptr);
    ~WatchDlg();
    void updateScreen(QImage &img);
protected:
    void closeEvent(QCloseEvent* event) override;



signals:
    void lockBtnClicked();
    void unLockBtnClicked();
    void stopWatch();
    void sigSendMouseData(const MOUSEEV& ev);

private:
    Ui::WatchDlg *ui;

};

#endif // WATCHDLG_H
