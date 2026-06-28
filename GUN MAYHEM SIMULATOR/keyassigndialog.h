#ifndef KEYASSIGNDIALOG_H
#define KEYASSIGNDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

class KeyAssignDialog : public QDialog {
public:
    int assignedKey = 0;

    KeyAssignDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Waiting for Input");
        setFixedSize(250, 100);

        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Press a valid key to assign...", this);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        // 过滤掉单纯的 Shift/Ctrl 等修饰键
        if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control || event->key() == Qt::Key_Alt) {
            return;
        }
        assignedKey = event->key();
        accept(); // 记录按键并关闭弹窗
    }
};

#endif // KEYASSIGNDIALOG_H