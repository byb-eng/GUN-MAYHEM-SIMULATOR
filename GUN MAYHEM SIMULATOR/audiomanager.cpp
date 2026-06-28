#include "audiomanager.h"

#include <QUrl>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <algorithm>

AudioManager& AudioManager::instance()
{
    static AudioManager mgr;
    return mgr;
}

AudioManager::AudioManager(QObject *parent)
    : QObject(parent),
      m_shootPlayer(new QMediaPlayer(this)),
      m_shootOutput(new QAudioOutput(this)),
      m_explosionPlayer(new QMediaPlayer(this)),
      m_explosionOutput(new QAudioOutput(this)),
      m_musicPlayer(new QMediaPlayer(this)),
      m_musicOutput(new QAudioOutput(this)),
      m_effectsVolume(0.62f),
      m_musicVolume(0.34f),
      m_musicEnabled(true),
      m_effectsEnabled(true)
{
    // 射击音效
    m_shootPlayer->setAudioOutput(m_shootOutput);
    m_shootOutput->setVolume(m_effectsVolume);
    m_shootPlayer->setSource(QUrl("qrc:/assets/射击声.m4a"));

    // 爆炸音效
    m_explosionPlayer->setAudioOutput(m_explosionOutput);
    m_explosionOutput->setVolume(m_effectsVolume);
    m_explosionPlayer->setSource(QUrl("qrc:/assets/爆炸声.m4a"));

    // 背景音乐
    m_musicPlayer->setAudioOutput(m_musicOutput);
    m_musicOutput->setVolume(m_musicVolume);

    QStringList musicPaths;
    musicPaths << QCoreApplication::applicationDirPath() + "/assets/background_music.m4a"
               << QDir::current().absoluteFilePath("assets/background_music.m4a");

    bool externalLoaded = false;
    for (const QString &path : musicPaths) {
        if (QFileInfo::exists(path)) {
            m_musicPlayer->setSource(QUrl::fromLocalFile(path));
            externalLoaded = true;
            break;
        }
    }
    if (!externalLoaded) {
        m_musicPlayer->setSource(QUrl("qrc:/assets/background_music.m4a"));
    }

    m_musicPlayer->setLoops(QMediaPlayer::Infinite);
}

void AudioManager::playOneShot(QMediaPlayer *player)
{
    if (!m_effectsEnabled || !player) {
        return;
    }
    player->stop();
    player->setPosition(0);
    player->play();
}

void AudioManager::playShoot()     { playOneShot(m_shootPlayer); }
void AudioManager::playExplosion() { playOneShot(m_explosionPlayer); }

void AudioManager::playBackgroundMusic()
{
    if (!m_musicEnabled || !m_musicPlayer) {
        return;
    }
    m_musicPlayer->play();
}

void AudioManager::stopBackgroundMusic()
{
    if (m_musicPlayer) {
        m_musicPlayer->pause();
    }
}

void AudioManager::setBackgroundMusicEnabled(bool enabled)
{
    m_musicEnabled = enabled;
    if (m_musicOutput) {
        m_musicOutput->setVolume(m_musicEnabled ? m_musicVolume : 0.0f);
    }
    if (m_musicEnabled) {
        playBackgroundMusic();
    } else {
        stopBackgroundMusic();
    }
}

bool AudioManager::isBackgroundMusicEnabled() const
{
    return m_musicEnabled;
}

void AudioManager::setEffectsEnabled(bool enabled)
{
    m_effectsEnabled = enabled;
    setEffectsVolume(m_effectsVolume);
}

bool AudioManager::isEffectsEnabled() const
{
    return m_effectsEnabled;
}

void AudioManager::setEffectsVolume(float volume)
{
    m_effectsVolume = std::clamp(volume, 0.0f, 1.0f);
    float realVolume = m_effectsEnabled ? m_effectsVolume : 0.0f;
    if (m_shootOutput) m_shootOutput->setVolume(realVolume);
    if (m_explosionOutput) m_explosionOutput->setVolume(realVolume);
}

float AudioManager::effectsVolume() const
{
    return m_effectsVolume;
}

void AudioManager::setMusicVolume(float volume)
{
    m_musicVolume = std::clamp(volume, 0.0f, 1.0f);
    if (m_musicOutput) {
        m_musicOutput->setVolume(m_musicEnabled ? m_musicVolume : 0.0f);
    }
}

float AudioManager::musicVolume() const
{
    return m_musicVolume;
}
