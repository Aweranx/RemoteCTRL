#ifndef SCREENDLG_H
#define SCREENDLG_H

#include <QWidget>

namespace Ui {
class ScreenDlg;
}

class ScreenDlg : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenDlg(QWidget *parent = nullptr);
    ~ScreenDlg();

private:
    Ui::ScreenDlg *ui;
};

#endif // SCREENDLG_H
