#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QSoundEffect>
#include <QUrl>
#include <QElapsedTimer>
#include <QString>
#include <functional>

class SoundManager : public QObject {
    Q_OBJECT
public:
    static SoundManager& instance();

    void playWaka();
    void playFruit();
    void playEatGhost();
    void playDeath();
    void playVictory();
    void playPeek();
    void playClick();

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);
    bool toggleMute();

    float volume() const { return m_volume; }
    void setVolume(float vol);

private:
    explicit SoundManager(QObject *parent = nullptr);
    ~SoundManager() override;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    void ensureSoundFilesGenerated();
    void createWav(const QString &filePath, int sampleRate, float duration,
                   const std::function<void(float, float, float&, float&)> &synth);

    bool m_muted;
    float m_volume;
    bool m_wakaToggle;
    QElapsedTimer m_wakaTimer;

    QSoundEffect *m_sndWaka1;
    QSoundEffect *m_sndWaka2;
    QSoundEffect *m_sndFruit;
    QSoundEffect *m_sndEatGhost;
    QSoundEffect *m_sndDeath;
    QSoundEffect *m_sndVictory;
    QSoundEffect *m_sndPeek;
    QSoundEffect *m_sndClick;
};

#endif // SOUNDMANAGER_H
