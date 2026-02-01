#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QString>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

struct AudioSessionData {
    DWORD pid;
    QString exePath;
    float volume; // 0.0 - 1.0
};

class AudioController : public QObject {
    Q_OBJECT
public:
    explicit AudioController(QObject *parent = nullptr);
    ~AudioController();

    void start();

public slots:
    void setVolume(DWORD pid, int volumePercent);
    void refresh();

signals:
    void sessionAdded(const AudioSessionData &data);
    void sessionRemoved(DWORD pid);

private:
    struct InternalSession {
        ISimpleAudioVolume *volume;
        QString exePath;
    };

    QMap<DWORD, InternalSession> m_sessions;
    QTimer *m_timer;

    bool shouldFilterOut(DWORD pid, ISimpleAudioVolume *volume, const QString &path);
    QString getProcessPath(DWORD pid);
};
