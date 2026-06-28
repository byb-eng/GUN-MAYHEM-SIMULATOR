#include "mainwindow.h"
#include "gamewidget.h"
#include "gameconfig.h"
#include "keyassigndialog.h"
#include "helpdialog.h"
#include "audiomanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <QKeySequence>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
#include <QDialog>
#include <QCheckBox>
#include <QSlider>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setFixedSize(960, 600);
    setWindowTitle("Gun Mayhem Simulator");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    m_menuWidget = new QWidget(this);
    m_gameWidget = new GameWidget(this);

    setupMenu();

    m_stackedWidget->addWidget(m_menuWidget);
    m_stackedWidget->addWidget(m_gameWidget);

    connect(m_gameWidget, &GameWidget::sigGoToMainMenu, this, [=]() {
        m_gameWidget->stopGame();
        AudioManager::instance().playBackgroundMusic();
        m_stackedWidget->setCurrentIndex(0);
    });

    AudioManager::instance().playBackgroundMusic();
}

MainWindow::~MainWindow() {}

void MainWindow::setupMenu()
{
    m_menuWidget->setObjectName("menuWidget");
    m_menuWidget->setStyleSheet(
        "QWidget#menuWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #1a1a2e, stop:0.3 #16213e, stop:0.7 #0f3460, stop:1.0 #533483);"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(m_menuWidget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    QLabel *title = new QLabel("GUN MAYHEM\nSIMULATOR", m_menuWidget);
    title->setFont(QFont("Arial Black", 42, QFont::Bold));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: #ffffff; letter-spacing: 3px;");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(title);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(255, 120, 50, 180));
    shadow->setOffset(0, 4);
    title->setGraphicsEffect(shadow);

    layout->addWidget(title);
    layout->addSpacing(20);

    QLabel *subtitle = new QLabel("Single Player AI / Local 2-Player Battle Arena", m_menuWidget);
    subtitle->setFont(QFont("Arial", 13));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color: #aabbcc;");
    layout->addWidget(subtitle);
    layout->addSpacing(40);

    QString btnStyle =
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #e94560, stop:1 #c23152);"
        "  color: white;"
        "  border: 2px solid #ff6b6b;"
        "  border-radius: 8px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #ff6b6b, stop:1 #e94560);"
        "  border: 2px solid #ff8888;"
        "}"
        "QPushButton:pressed {"
        "  background: #a02040;"
        "  border: 2px solid #e94560;"
        "}";

    QStringList btnNames = {"开始游戏", "游戏设置", "游戏帮助", "制作成员"};
    for (int i = 0; i < btnNames.size(); ++i) {
        QPushButton *btn = new QPushButton(btnNames[i], m_menuWidget);
        btn->setFixedSize(220, 52);
        btn->setStyleSheet(btnStyle);
        btn->setCursor(Qt::PointingHandCursor);
        layout->addWidget(btn, 0, Qt::AlignCenter);

        if (i == 0) {
            connect(btn, &QPushButton::clicked, this, &MainWindow::showModeDialog);
        } else if (i == 1) {
            connect(btn, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
        } else if (i == 2) {
            connect(btn, &QPushButton::clicked, this, [=]() {
                HelpDialog dialog(this);
                connect(&dialog, &HelpDialog::surpriseClicked, this, &MainWindow::showCreditsDialog);
                dialog.exec();
            });
        } else if (i == 3) {
            connect(btn, &QPushButton::clicked, this, &MainWindow::showCreditsDialog);
        }
    }
}

void MainWindow::showModeDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("选择游戏模式");
    dialog.setFixedSize(320, 260);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("请选择游戏模式", &dialog);
    label->setAlignment(Qt::AlignCenter);
    label->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));

    QPushButton *btnSimple = new QPushButton("单机（简单 AI）", &dialog);
    QPushButton *btnHard = new QPushButton("单机（困难 AI）", &dialog);
    QPushButton *btnPvp = new QPushButton("双人对战", &dialog);
    QPushButton *btnCancel = new QPushButton("取消", &dialog);

    layout->addWidget(label);
    layout->addSpacing(10);
    layout->addWidget(btnSimple);
    layout->addWidget(btnHard);
    layout->addWidget(btnPvp);
    layout->addSpacing(8);
    layout->addWidget(btnCancel);

    connect(btnSimple, &QPushButton::clicked, &dialog, [&]() {
        dialog.done(static_cast<int>(GameMode::HumanVsSimpleAI));
    });
    connect(btnHard, &QPushButton::clicked, &dialog, [&]() {
        dialog.done(static_cast<int>(GameMode::HumanVsHardAI));
    });
    connect(btnPvp, &QPushButton::clicked, &dialog, [&]() {
        dialog.done(static_cast<int>(GameMode::HumanVsHuman));
    });
    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    int result = dialog.exec();
    if (result == static_cast<int>(GameMode::HumanVsSimpleAI) ||
        result == static_cast<int>(GameMode::HumanVsHardAI) ||
        result == static_cast<int>(GameMode::HumanVsHuman)) {
        startGame(static_cast<GameMode>(result));
    }
}

void MainWindow::startGame(GameMode mode)
{
    AudioManager::instance().playBackgroundMusic();
    m_gameWidget->startGame(mode);
    m_stackedWidget->setCurrentIndex(1);
    m_gameWidget->setFocus();
}


void MainWindow::showSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("游戏设置");
    dialog.setFixedSize(600, 560);

    QVBoxLayout *outerLayout = new QVBoxLayout(&dialog);

    QGroupBox *audioBox = new QGroupBox("声音设置", &dialog);
    QVBoxLayout *audioLayout = new QVBoxLayout(audioBox);

    QCheckBox *musicCheck = new QCheckBox("开启背景音乐", audioBox);
    musicCheck->setChecked(AudioManager::instance().isBackgroundMusicEnabled());
    audioLayout->addWidget(musicCheck);

    QHBoxLayout *musicVolLayout = new QHBoxLayout();
    QLabel *musicLabel = new QLabel("背景音乐音量", audioBox);
    QSlider *musicSlider = new QSlider(Qt::Horizontal, audioBox);
    QLabel *musicValue = new QLabel(audioBox);
    musicSlider->setRange(0, 100);
    musicSlider->setValue(static_cast<int>(AudioManager::instance().musicVolume() * 100));
    musicValue->setText(QString::number(musicSlider->value()) + "%");
    musicVolLayout->addWidget(musicLabel);
    musicVolLayout->addWidget(musicSlider);
    musicVolLayout->addWidget(musicValue);
    audioLayout->addLayout(musicVolLayout);

    QCheckBox *effectsCheck = new QCheckBox("开启音效", audioBox);
    effectsCheck->setChecked(AudioManager::instance().isEffectsEnabled());
    audioLayout->addWidget(effectsCheck);

    QHBoxLayout *effectVolLayout = new QHBoxLayout();
    QLabel *effectLabel = new QLabel("音效音量", audioBox);
    QSlider *effectSlider = new QSlider(Qt::Horizontal, audioBox);
    QLabel *effectValue = new QLabel(audioBox);
    effectSlider->setRange(0, 100);
    effectSlider->setValue(static_cast<int>(AudioManager::instance().effectsVolume() * 100));
    effectValue->setText(QString::number(effectSlider->value()) + "%");
    effectVolLayout->addWidget(effectLabel);
    effectVolLayout->addWidget(effectSlider);
    effectVolLayout->addWidget(effectValue);
    audioLayout->addLayout(effectVolLayout);

    outerLayout->addWidget(audioBox);

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

    QHBoxLayout *mainLayout = new QHBoxLayout();
    outerLayout->addLayout(mainLayout);

    auto createKeyGroup = [&](const QString &title, PlayerKeys &keys) -> QGroupBox* {
        QGroupBox *box = new QGroupBox(title);
        QGridLayout *grid = new QGridLayout(box);

        struct KeyMap { QString name; int* keyRef; int row; int col; };
        std::vector<KeyMap> maps = {
            {"跳跃", &keys.up, 0, 1},
            {"左", &keys.left, 1, 0},
            {"下降", &keys.down, 1, 1},
            {"右", &keys.right, 1, 2},
            {"射击", &keys.shoot, 2, 0},
            {"炸弹", &keys.bomb, 2, 1}
        };

        for (auto &m : maps) {
            QPushButton *btn = new QPushButton(QKeySequence(*m.keyRef).toString());
            btn->setFixedSize(50, 50);

            connect(btn, &QPushButton::clicked, [m, btn, &dialog]() {
                KeyAssignDialog assignDlg(&dialog);
                if (assignDlg.exec() == QDialog::Accepted) {
                    *m.keyRef = assignDlg.assignedKey;
                    btn->setText(QKeySequence(*m.keyRef).toString());
                }
            });

            QLabel *label = new QLabel(m.name);
            label->setAlignment(Qt::AlignCenter);

            QVBoxLayout *vbox = new QVBoxLayout();
            vbox->addWidget(label);
            vbox->addWidget(btn);
            grid->addLayout(vbox, m.row, m.col);
        }
        return box;
    };

    mainLayout->addWidget(createKeyGroup("玩家 1", GameConfig::instance().p1));
    mainLayout->addWidget(createKeyGroup("玩家 2", GameConfig::instance().p2));

    QPushButton *btnClose = new QPushButton("关闭", &dialog);
    outerLayout->addWidget(btnClose, 0, Qt::AlignCenter);
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::showCreditsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("制作成员");
    dialog.setFixedSize(520, 480);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setAlignment(Qt::AlignCenter);

    // 团队照片
    QLabel *photoLabel = new QLabel;
    QPixmap photo(":/assets/we_there.png");
    photoLabel->setPixmap(photo.scaled(480, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    photoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(photoLabel);

    // 标题
    QLabel *titleLabel = new QLabel("制作团队");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-top: 10px;");
    layout->addWidget(titleLabel);

    dialog.exec();
}
