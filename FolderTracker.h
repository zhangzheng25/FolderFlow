#ifndef FOLDERTRACKER_H
#define FOLDERTRACKER_H

// FolderTracker.h (精简版逻辑)
#include <Windows.h>
#include <ShlGuid.h>
#include <ExDisp.h>
#include <Shlobj.h>
#include <atlbase.h>
#include <QTimer>
#include <QObject>
#include <DatabaseManager.h>
#include <QDebug>

class FolderTracker : public QObject {
    Q_OBJECT
private:
    QString m_lastPath;
    int m_stayMs = 0;
    bool m_loggedCurrentPath = false;
    static constexpr int SCAN_INTERVAL_MS = 500;
    static constexpr int STAY_THRESHOLD_MS = 2000;

signals:
    void updateList();

public:
    FolderTracker() {
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &FolderTracker::scanCurrentFolder);
        timer->start(SCAN_INTERVAL_MS);
    }

private:
    void scanCurrentFolder() {
        QString currentActivePath = getForegroundExplorerPath();

        if (currentActivePath.isEmpty()) {
            m_lastPath.clear();
            m_stayMs = 0;
            m_loggedCurrentPath = false;
            return;
        }

        if (currentActivePath == m_lastPath) {
            m_stayMs += SCAN_INTERVAL_MS;
        } else {
            m_lastPath = currentActivePath;
            m_stayMs = SCAN_INTERVAL_MS;
            m_loggedCurrentPath = false;
        }

        if (!m_loggedCurrentPath && m_stayMs >= STAY_THRESHOLD_MS) {
            DatabaseManager::logAccess(m_lastPath);
            m_loggedCurrentPath = true;
            emit updateList();
        }
    }

    QString getForegroundExplorerPath() {
        CComPtr<IShellWindows> psw;
        if (FAILED(psw.CoCreateInstance(CLSID_ShellWindows))) return "";

        long count = 0;
        if (FAILED(psw->get_Count(&count))) return "";

        // 获取当前前台窗口句柄
        HWND foregroundHwnd = GetForegroundWindow();

        for (long i = 0; i < count; ++i) {
            CComVariant v(i);
            CComPtr<IDispatch> pdisp;
            if (FAILED(psw->Item(v, &pdisp)) || !pdisp) continue;

            CComQIPtr<IWebBrowser2> pwb(pdisp);
            if (!pwb) continue;

            HWND hwndBrowser = NULL;
            if (FAILED(pwb->get_HWND((SHANDLE_PTR*)&hwndBrowser))) continue;

            // 只处理当前处于前台的资源管理器窗口
            if (hwndBrowser == foregroundHwnd) {
                CComPtr<IServiceProvider> psp;
                if (FAILED(pwb->QueryInterface(IID_IServiceProvider, (void**)&psp)) || !psp) continue;

                CComPtr<IShellBrowser> psb;
                if (FAILED(psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb)) || !psb) continue;

                CComPtr<IShellView> psv;
                if (FAILED(psb->QueryActiveShellView(&psv)) || !psv) continue;

                CComQIPtr<IFolderView> pfv(psv);
                if (!pfv) continue;

                CComPtr<IPersistFolder2> ppf2;
                if (FAILED(pfv->GetFolder(IID_IPersistFolder2, (void**)&ppf2)) || !ppf2) continue;

                LPITEMIDLIST pidl = NULL;
                if (FAILED(ppf2->GetCurFolder(&pidl)) || !pidl) continue;

                wchar_t path[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, path)) {
                    CoTaskMemFree(pidl);
                    return QString::fromWCharArray(path);
                }
                CoTaskMemFree(pidl);
            }
        }
        return "";
    }
};

#endif // FOLDERTRACKER_H
