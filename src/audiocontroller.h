#pragma once

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QString>
#include <QSettings>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <QFileInfo>
#include <QSet>
#include <QDebug>

struct AudioSessionData {
    DWORD pid;
    QString exePath;
    float volume; // 0.0 - 1.0
};

class AudioController : public QObject {
    Q_OBJECT
public:
    explicit AudioController(QObject *parent = nullptr);
    AudioController(QSettings *settings, QObject *parent = nullptr); // 新增带配置的构造函数
    ~AudioController();

    void start();
    void setSettings(QSettings *settings); // 支持动态更新配置

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
    QSettings *m_settings = nullptr; // 配置
    QStringList m_skipNames; // 缓存的屏蔽列表

    bool shouldFilterOut(DWORD pid, ISimpleAudioVolume *volume, const QString &path);
    void loadSettings(); // 加载配置
    QString getProcessPath(DWORD pid);
};
