#include "audiocontroller.h"
#include <QFileInfo>
#include <QSet>
#include <QDebug>

AudioController::AudioController(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AudioController::refresh);
}

AudioController::~AudioController() {
    // 释放所有 COM 接口指针
    for (auto &session : m_sessions) {
        if (session.volume) session.volume->Release();
    }
    m_sessions.clear();
}

void AudioController::start() {
    // 启动定时刷新 (每 100ms)
    m_timer->start(100);
}

void AudioController::setVolume(DWORD pid, int volumePercent) {
    if (m_sessions.contains(pid)) {
        ISimpleAudioVolume *vol = m_sessions[pid].volume;
        if (vol) {
            // 设定主音量 0.0 - 1.0 (内部转换百分比)
            vol->SetMasterVolume(volumePercent / 100.0f, nullptr);
        }
    }
}

// 获取 PID 对应的进程完整路径
QString AudioController::getProcessPath(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc)
        return {};

    wchar_t exePath[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameW(hProc, 0, exePath, &size);
    CloseHandle(hProc);

    return QString::fromWCharArray(exePath);
}

// 过滤不需要显示的音频会话
bool AudioController::shouldFilterOut(DWORD pid, ISimpleAudioVolume *volume, const QString &exePath) {
    if (exePath.isEmpty())
        return true;

    QString exeName = QFileInfo(exePath).fileName().toLower();

    // 默认黑名单，屏蔽系统服务或无意义进程
    static const QStringList defaultSkipNames = {
        "system", "idle", "audiodg.exe", "runtimebroker.exe"};
    if (defaultSkipNames.contains(exeName))
        return true;

    float selfVol = 0.0f;
    if (FAILED(volume->GetMasterVolume(&selfVol)))
        return true;

    // 判断是否已有同名进程且有声音(自动隐藏手动静音的同名进程)
    bool sameNameWithSound = false;
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it.key() == pid) continue;

        QString otherName = QFileInfo(it.value().exePath).fileName().toLower();
        if (otherName == exeName) {
            ISimpleAudioVolume *otherVol = it.value().volume;
            if (otherVol) {
                float otherVolume = 0.0f;
                if (SUCCEEDED(otherVol->GetMasterVolume(&otherVolume)) && otherVolume > 0.01f) {
                    sameNameWithSound = true;
                    break;
                }
            }
        }
    }

    return (selfVol <= 0.01f && sameNameWithSound);
}

void AudioController::refresh() {
    // 记录本次扫描到的 PID
    QSet<DWORD> foundPIDs;
    
    // Windows Core Audio APIs 接口
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioSessionManager2 *sessionManager = nullptr;
    IAudioSessionEnumerator *sessionEnumerator = nullptr;

    // 1. 初始化设备枚举器
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                                 __uuidof(IMMDeviceEnumerator), (void **)&deviceEnumerator))) return;

    // 2. 获取默认音频渲染设备
    if (FAILED(deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device))) {
        deviceEnumerator->Release();
        return;
    }

    // 3. 激活音频会话管理器
    if (FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                                 (void **)&sessionManager))) {
        device->Release();
        deviceEnumerator->Release();
        return;
    }

    // 4. 获取会话枚举器
    if (FAILED(sessionManager->GetSessionEnumerator(&sessionEnumerator))) {
        sessionManager->Release();
        device->Release();
        deviceEnumerator->Release();
        return;
    }

    int count = 0;
    sessionEnumerator->GetCount(&count);

    // 5. 遍历所有会话
    for (int i = 0; i < count; ++i) {
        IAudioSessionControl *ctrl = nullptr;
        if (FAILED(sessionEnumerator->GetSession(i, &ctrl))) continue;

        IAudioSessionControl2 *ctrl2 = nullptr;
        if (FAILED(ctrl->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&ctrl2))) {
            ctrl->Release();
            continue;
        }

        DWORD pid = 0;
        if (FAILED(ctrl2->GetProcessId(&pid))) {
            ctrl2->Release();
            ctrl->Release();
            continue;
        }

        ISimpleAudioVolume *volume = nullptr;
        if (FAILED(ctrl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&volume))) {
            ctrl2->Release();
            ctrl->Release();
            continue;
        }

        // 检查是否应该保留此会话
        QString path = getProcessPath(pid);
        bool keep = !shouldFilterOut(pid, volume, path);

        if (keep) {
            foundPIDs.insert(pid);
            if (!m_sessions.contains(pid)) {
                // 新发现的会话
                m_sessions.insert(pid, {volume, path});
                float vol = 0.0f;
                volume->GetMasterVolume(&vol);
                emit sessionAdded({pid, path, vol});
            } else {
                // 已存在的会话，释放新的接口指针，保留旧的
                volume->Release();
            }
        } else {
            // 被过滤掉的会话
            volume->Release();
        }

        ctrl2->Release();
        ctrl->Release();
    }

    // 释放资源
    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    deviceEnumerator->Release();

    // 移除已经消失的会话
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (!foundPIDs.contains(it.key())) {
            emit sessionRemoved(it.key());
            if (it.value().volume) it.value().volume->Release();
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}
