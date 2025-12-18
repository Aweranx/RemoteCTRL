#include "screendlg.h"
#include "ui_screendlg.h"

ScreenDlg::ScreenDlg(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScreenDlg)
{
    ui->setupUi(this);
}

ScreenDlg::~ScreenDlg()
{
    delete ui;
}
