#include "watchdlg.h"
#include "ui_watchdlg.h"
#include <QPushButton>
#include <QCloseEvent>

WatchDlg::WatchDlg(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WatchDlg)
{
    ui->setupUi(this);
    connect(ui->lockBtn, &QPushButton::clicked, this, &WatchDlg::lockBtnClicked);
    connect(ui->unLockBtn, &QPushButton::clicked, this, &WatchDlg::unLockBtnClicked);
}

void WatchDlg::closeEvent(QCloseEvent* event) {
    emit stopWatch();
    event->accept();
}

WatchDlg::~WatchDlg()
{
    delete ui;
}

void WatchDlg::updateScreen(QImage &img)
{
    ui->watchWidget->updateImage(img);
}
