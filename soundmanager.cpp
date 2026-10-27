#include "soundmanager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <cmath>
#include <vector>
#include <algorithm>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4]  = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = 22050;
    uint32_t byteRate = 44100;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};
#pragma pack(pop)

SoundManager& SoundManager::instance()
{
    static SoundManager s_instance;
    return s_instance;
}

SoundManager::SoundManager(QObject *parent)
    : QObject(parent)
    , m_muted(false)
    , m_volume(0.80f)
    , m_wakaToggle(false)
    , m_sndWaka1(nullptr)
    , m_sndWaka2(nullptr)
    , m_sndFruit(nullptr)
    , m_sndEatGhost(nullptr)
    , m_sndDeath(nullptr)
    , m_sndVictory(nullptr)
    , m_sndPeek(nullptr)
    , m_sndClick(nullptr)
{
    QSettings settings("PacmanGame", "LabyrinthSaga");
    m_muted = settings.value("audioMuted", false).toBool();
    m_volume = settings.value("audioVolume", 0.80f).toFloat();

    m_wakaTimer.start();
    ensureSoundFilesGenerated();
}

SoundManager::~SoundManager()
{
}

void SoundManager::createWav(const QString &filePath, int sampleRate, float duration,
                             const std::function<void(float, float, float&, float&)> &synth)
{
    int numSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> buffer(numSamples, 0);

    float dt = 1.0f / static_cast<float>(sampleRate);
    float phase = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        float t = i * dt;
        float sample = 0.0f;
        synth(t, dt, sample, phase);
        sample = std::clamp(sample, -1.0f, 1.0f);
        buffer[i] = static_cast<int16_t>(sample * 32760.0f);
    }

    WavHeader header;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * 2;
    header.dataSize = numSamples * sizeof(int16_t);
    header.fileSize = sizeof(WavHeader) - 8 + header.dataSize;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(int16_t));
        file.close();
    }
}

void SoundManager::ensureSoundFilesGenerated()
{
    QString soundDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds";
    QDir().mkpath(soundDir);

    const int sampleRate = 22050;

    // 1. Waka 1: Retro upward pitch sweep (240Hz -> 480Hz)
    QString waka1Path = soundDir + "/waka1.wav";
    if (!QFile::exists(waka1Path) || QFile(waka1Path).size() == 0) {
        createWav(waka1Path, sampleRate, 0.085f, [](float t, float dt, float &sample, float &phase) {
            float duration = 0.085f;
            float freq = 240.0f + 250.0f * (t / duration);
            phase += 2.0f * M_PI * freq * dt;
            float env = std::sin((t / duration) * M_PI);
            sample = std::sin(phase) * env * 0.9f;
        });
    }

    // 2. Waka 2: Retro downward pitch sweep (480Hz -> 240Hz)
    QString waka2Path = soundDir + "/waka2.wav";
    if (!QFile::exists(waka2Path) || QFile(waka2Path).size() == 0) {
        createWav(waka2Path, sampleRate, 0.085f, [](float t, float dt, float &sample, float &phase) {
            float duration = 0.085f;
            float freq = 490.0f - 240.0f * (t / duration);
            phase += 2.0f * M_PI * freq * dt;
            float env = std::sin((t / duration) * M_PI);
            sample = std::sin(phase) * env * 0.9f;
        });
    }

    // 3. Fruit Pick: Joyous 4-step ascending arpeggio (C5 -> E5 -> G5 -> C6)
    QString fruitPath = soundDir + "/fruit.wav";
    if (!QFile::exists(fruitPath) || QFile(fruitPath).size() == 0) {
        createWav(fruitPath, sampleRate, 0.32f, [](float t, float dt, float &sample, float &phase) {
            float noteT = std::fmod(t, 0.08f);
            int step = std::min(3, static_cast<int>(t / 0.08f));
            float freqs[4] = {523.25f, 659.25f, 783.99f, 1046.50f};
            float freq = freqs[step];
            phase += 2.0f * M_PI * freq * dt;
            float env = std::exp(-14.0f * noteT);
            float wave = 0.75f * std::sin(phase) + 0.25f * std::sin(phase * 2.0f);
            sample = wave * env * 0.85f;
        });
    }

    // 4. Eat Ghost: Arcade crunchy ascending chord with low rumble
    QString eatGhostPath = soundDir + "/eat_ghost.wav";
    if (!QFile::exists(eatGhostPath) || QFile(eatGhostPath).size() == 0) {
        createWav(eatGhostPath, sampleRate, 0.42f, [](float t, float dt, float &sample, float &phase) {
            float duration = 0.42f;
            float freq = 180.0f + 650.0f * std::pow(t / duration, 1.4f);
            freq += 60.0f * std::sin(2.0f * M_PI * 45.0f * t);
            phase += 2.0f * M_PI * freq * dt;
            float env = (t < 0.03f) ? (t / 0.03f) : (1.0f - (t / duration));
            float square = (std::sin(phase) > 0.0f) ? 0.5f : -0.5f;
            float wave = 0.55f * square + 0.45f * std::sin(phase);
            sample = wave * env * 0.90f;
        });
    }

    // 5. Death: Descending 10-step cartoon wobble down to low bloop
    QString deathPath = soundDir + "/death.wav";
    if (!QFile::exists(deathPath) || QFile(deathPath).size() == 0) {
        createWav(deathPath, sampleRate, 1.05f, [](float t, float dt, float &sample, float &phase) {
            if (t < 0.85f) {
                float stepDuration = 0.085f;
                int step = std::min(9, static_cast<int>(t / stepDuration));
                float tInStep = t - step * stepDuration;
                float startFreq = 620.0f - step * 48.0f;
                float freq = startFreq - (tInStep / stepDuration) * 60.0f;
                phase += 2.0f * M_PI * freq * dt;
                float env = std::sin((tInStep / stepDuration) * M_PI);
                sample = std::sin(phase) * env * 0.85f;
            } else {
                float tEnd = t - 0.85f;
                float freq = 110.0f - tEnd * 200.0f;
                if (freq < 40.0f) freq = 40.0f;
                phase += 2.0f * M_PI * freq * dt;
                float env = std::exp(-15.0f * tEnd);
                sample = std::sin(phase) * env * 0.80f;
            }
        });
    }

    // 6. Victory: Triumphant 8-bit fanfare (C5-E5-G5-C6... G5-C6!)
    QString victoryPath = soundDir + "/victory.wav";
    if (!QFile::exists(victoryPath) || QFile(victoryPath).size() == 0) {
        createWav(victoryPath, sampleRate, 1.25f, [](float t, float dt, float &sample, float &phase) {
            float freq = 523.25f;
            float env = 1.0f;
            if (t < 0.12f) {
                freq = 523.25f; // C5
                env = std::exp(-4.0f * t);
            } else if (t < 0.24f) {
                freq = 659.25f; // E5
                env = std::exp(-4.0f * (t - 0.12f));
            } else if (t < 0.36f) {
                freq = 783.99f; // G5
                env = std::exp(-4.0f * (t - 0.24f));
            } else if (t < 0.55f) {
                freq = 1046.50f; // C6
                env = std::exp(-3.0f * (t - 0.36f));
            } else if (t < 0.65f) {
                freq = 0.0f;
                env = 0.0f;
            } else if (t < 0.78f) {
                freq = 783.99f; // G5
                env = std::exp(-4.0f * (t - 0.65f));
            } else {
                freq = 1046.50f; // C6 sustained with shimmer
                float tEnd = t - 0.78f;
                freq += 15.0f * std::sin(2.0f * M_PI * 6.0f * tEnd);
                env = std::max(0.0f, 1.0f - (tEnd / 0.47f));
            }
            if (freq > 0.0f) {
                phase += 2.0f * M_PI * freq * dt;
                float wave = 0.7f * std::sin(phase) + 0.3f * std::sin(phase * 2.0f);
                sample = wave * env * 0.90f;
            } else {
                sample = 0.0f;
            }
        });
    }

    // 7. Peek: Mystical magical lantern chime (Harmonic high shimmer)
    QString peekPath = soundDir + "/peek.wav";
    if (!QFile::exists(peekPath) || QFile(peekPath).size() == 0) {
        createWav(peekPath, sampleRate, 0.28f, [](float t, float dt, float &sample, float &phase) {
            float freq1 = 1318.5f; // E6
            float freq2 = 1760.0f; // A6
            phase += 2.0f * M_PI * freq1 * dt;
            float env = std::exp(-12.0f * t);
            float wave = 0.55f * std::sin(phase) + 0.45f * std::sin(phase * (freq2 / freq1));
            sample = wave * env * 0.75f;
        });
    }

    // 8. Click: Crisp 40ms micro UI click
    QString clickPath = soundDir + "/click.wav";
    if (!QFile::exists(clickPath) || QFile(clickPath).size() == 0) {
        createWav(clickPath, sampleRate, 0.04f, [](float t, float dt, float &sample, float &phase) {
            float duration = 0.04f;
            float freq = 880.0f - 500.0f * (t / duration);
            phase += 2.0f * M_PI * freq * dt;
            float env = std::exp(-70.0f * t);
            sample = std::sin(phase) * env * 0.70f;
        });
    }

    // Initialize QSoundEffect instances
    auto setupEffect = [this](QSoundEffect* &effect, const QString &path) {
        if (!effect) {
            effect = new QSoundEffect(this);
            effect->setSource(QUrl::fromLocalFile(path));
            effect->setVolume(m_muted ? 0.0f : m_volume);
        }
    };

    setupEffect(m_sndWaka1, waka1Path);
    setupEffect(m_sndWaka2, waka2Path);
    setupEffect(m_sndFruit, fruitPath);
    setupEffect(m_sndEatGhost, eatGhostPath);
    setupEffect(m_sndDeath, deathPath);
    setupEffect(m_sndVictory, victoryPath);
    setupEffect(m_sndPeek, peekPath);
    setupEffect(m_sndClick, clickPath);
}

void SoundManager::playWaka()
{
    if (m_muted) return;
    if (m_wakaTimer.isValid() && m_wakaTimer.elapsed() < 75) {
        return; // smooth throttle so rapid movement doesn't clip
    }
    m_wakaTimer.restart();

    if (m_wakaToggle) {
        if (m_sndWaka1) m_sndWaka1->play();
    } else {
        if (m_sndWaka2) m_sndWaka2->play();
    }
    m_wakaToggle = !m_wakaToggle;
}

void SoundManager::playFruit()
{
    if (m_muted) return;
    if (m_sndFruit) m_sndFruit->play();
}

void SoundManager::playEatGhost()
{
    if (m_muted) return;
    if (m_sndEatGhost) m_sndEatGhost->play();
}

void SoundManager::playDeath()
{
    if (m_muted) return;
    if (m_sndDeath) m_sndDeath->play();
}

void SoundManager::playVictory()
{
    if (m_muted) return;
    if (m_sndVictory) m_sndVictory->play();
}

void SoundManager::playPeek()
{
    if (m_muted) return;
    if (m_sndPeek) m_sndPeek->play();
}

void SoundManager::playClick()
{
    if (m_muted) return;
    if (m_sndClick) m_sndClick->play();
}

void SoundManager::setMuted(bool muted)
{
    m_muted = muted;
    float effVol = m_muted ? 0.0f : m_volume;
    if (m_sndWaka1) m_sndWaka1->setVolume(effVol);
    if (m_sndWaka2) m_sndWaka2->setVolume(effVol);
    if (m_sndFruit) m_sndFruit->setVolume(effVol);
    if (m_sndEatGhost) m_sndEatGhost->setVolume(effVol);
    if (m_sndDeath) m_sndDeath->setVolume(effVol);
    if (m_sndVictory) m_sndVictory->setVolume(effVol);
    if (m_sndPeek) m_sndPeek->setVolume(effVol);
    if (m_sndClick) m_sndClick->setVolume(effVol);

    QSettings settings("PacmanGame", "LabyrinthSaga");
    settings.setValue("audioMuted", m_muted);
}

bool SoundManager::toggleMute()
{
    setMuted(!m_muted);
    return m_muted;
}

void SoundManager::setVolume(float vol)
{
    m_volume = std::clamp(vol, 0.0f, 1.0f);
    if (!m_muted) {
        if (m_sndWaka1) m_sndWaka1->setVolume(m_volume);
        if (m_sndWaka2) m_sndWaka2->setVolume(m_volume);
        if (m_sndFruit) m_sndFruit->setVolume(m_volume);
        if (m_sndEatGhost) m_sndEatGhost->setVolume(m_volume);
        if (m_sndDeath) m_sndDeath->setVolume(m_volume);
        if (m_sndVictory) m_sndVictory->setVolume(m_volume);
        if (m_sndPeek) m_sndPeek->setVolume(m_volume);
        if (m_sndClick) m_sndClick->setVolume(m_volume);
    }
    QSettings settings("PacmanGame", "LabyrinthSaga");
    settings.setValue("audioVolume", m_volume);
}
