#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}

void HelpDialog::on_btn_surprise_clicked()
{
    emit surpriseClicked(); // 发出信号，通知主窗口
    accept();               // 关闭当前的帮助弹窗
}

