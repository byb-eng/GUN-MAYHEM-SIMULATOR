#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

class AudioManager : public QObject
{
    Q_OBJECT

public:
    static AudioManager& instance();

    void playShoot();
    void playExplosion();

    void playBackgroundMusic();
    void stopBackgroundMusic();

    void setBackgroundMusicEnabled(bool enabled);
    bool isBackgroundMusicEnabled() const;

    void setEffectsEnabled(bool enabled);
    bool isEffectsEnabled() const;

    void setEffectsVolume(float volume);
    float effectsVolume() const;

    void setMusicVolume(float volume);
    float musicVolume() const;

private:
    explicit AudioManager(QObject *parent = nullptr);

    void playOneShot(QMediaPlayer *player);

    QMediaPlayer *m_shootPlayer;
    QAudioOutput *m_shootOutput;

    QMediaPlayer *m_explosionPlayer;
    QAudioOutput *m_explosionOutput;

    QMediaPlayer *m_musicPlayer;
    QAudioOutput *m_musicOutput;

    float m_effectsVolume;
    float m_musicVolume;
    bool m_musicEnabled;
    bool m_effectsEnabled;
};

#endif // AUDIOMANAGER_H
