#ifndef PAUSEDIALOG_H
#define PAUSEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include "audiomanager.h"

class PauseDialog : public QDialog {
    Q_OBJECT
public:
    explicit PauseDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("游戏暂停");
        setFixedSize(340, 330);
        setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *title = new QLabel("游戏暂停", this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size: 18px; font-weight: bold;");
        layout->addWidget(title);

        QCheckBox *musicCheck = new QCheckBox("开启背景音乐", this);
        musicCheck->setChecked(AudioManager::instance().isBackgroundMusicEnabled());
        layout->addWidget(musicCheck);

        QHBoxLayout *musicVolLayout = new QHBoxLayout();
        QLabel *musicLabel = new QLabel("背景音乐音量", this);
        QSlider *musicSlider = new QSlider(Qt::Horizontal, this);
        QLabel *musicValue = new QLabel(this);
        musicSlider->setRange(0, 100);
        musicSlider->setValue(static_cast<int>(AudioManager::instance().musicVolume() * 100));
        musicValue->setText(QString::number(musicSlider->value()) + "%");
        musicVolLayout->addWidget(musicLabel);
        musicVolLayout->addWidget(musicSlider);
        musicVolLayout->addWidget(musicValue);
        layout->addLayout(musicVolLayout);

        QCheckBox *effectsCheck = new QCheckBox("开启音效", this);
        effectsCheck->setChecked(AudioManager::instance().isEffectsEnabled());
        layout->addWidget(effectsCheck);

        QHBoxLayout *effectVolLayout = new QHBoxLayout();
        QLabel *effectLabel = new QLabel("音效音量", this);
        QSlider *effectSlider = new QSlider(Qt::Horizontal, this);
        QLabel *effectValue = new QLabel(this);
        effectSlider->setRange(0, 100);
        effectSlider->setValue(static_cast<int>(AudioManager::instance().effectsVolume() * 100));
        effectValue->setText(QString::number(effectSlider->value()) + "%");
        effectVolLayout->addWidget(effectLabel);
        effectVolLayout->addWidget(effectSlider);
        effectVolLayout->addWidget(effectValue);
        layout->addLayout(effectVolLayout);

        QPushButton *btnResume = new QPushButton("继续游戏", this);
        QPushButton *btnRestart = new QPushButton("重新开始", this);
        QPushButton *btnMenu = new QPushButton("返回主菜单", this);

        layout->addSpacing(8);
        layout->addWidget(btnResume);
        layout->addWidget(btnRestart);
        layout->addWidget(btnMenu);

        connect(musicCheck, &QCheckBox::toggled, this, [](bool checked) {
            AudioManager::instance().setBackgroundMusicEnabled(checked);
        });
        connect(musicSlider, &QSlider::valueChanged, this, [=](int value) {
            AudioManager::instance().setMusicVolume(value / 100.0f);
            musicValue->setText(QString::number(value) + "%");
        });

        connect(effectsCheck, &QCheckBox::toggled, this, [](bool checked) {
            AudioManager::instance().setEffectsEnabled(checked);
        });
        connect(effectSlider, &QSlider::valueChanged, this, [=](int value) {
            AudioManager::instance().setEffectsVolume(value / 100.0f);
            effectValue->setText(QString::number(value) + "%");
        });

        connect(btnResume, &QPushButton::clicked, this, [=](){ done(1); });
        connect(btnRestart, &QPushButton::clicked, this, [=](){ done(2); });
        connect(btnMenu, &QPushButton::clicked, this, [=](){ done(3); });
    }
};

#endif // PAUSEDIALOG_H
