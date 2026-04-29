#ifndef CWRAPPER_H
#define CWRAPPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QEventLoop>
#include <QDateTime>
#include <QList>

#include "chathistoryitem.h"   // SessionInfo lives here


class CWrapper : public QObject
{
    Q_OBJECT
public:
    explicit CWrapper(QObject *parent = nullptr);
    ~CWrapper();

    // Called once the worker thread has started
    Q_INVOKABLE void init();

    void setServerIP(const QString& ip)     { _serverIP   = ip;   }
    void setServerPort(int port)            { _serverPort = port; }
    void setApiKey(const QString& key)      { _apiKey     = key;  }
    void setModelName(const QString& name)  { _modelName  = name; }
    void setSystemPrompt(const QString& sp) { systemPrompt = sp;  }

    QString getServerIP()   const { return _serverIP;   }
    int     getServerPort() const { return _serverPort; }
    QString getApiKey()     const { return _apiKey;     }
    QString getModelName()  const { return _modelName;  }
    bool    isModelLoaded() const { return model_loaded;}

    /* ── Session / history ──────────────────────────────────────────────────
     *
     *  listSessions(n)
     *    Static — safe to call from ANY thread, including the GUI thread.
     *    Scans the working directory for "history_*.cbor" files, opens each
     *    to read the first user message (preview), then returns the n most
     *    recent sessions sorted newest-first.
     *    Pass n = 0 to return all sessions.
     * ───────────────────────────────────────────────────────────────────── */
    static QList<SessionInfo> listSessions(int n = 0);

    /* Load a specific .cbor file into this instance's _history and replay
     * it into the chat UI.  Safe to call from the GUI thread before the
     * worker thread has touched anything.
     * Returns false if the file can't be read.                           */
    bool loadSessionFromPath(const QString& path);

    void    clearHistory();
    bool    saveHistory();
    bool    loadHistory();
    QString listModels();
    bool    loadModel();

    QString sessionPath() const { return history_file_path; }

public slots:
    void chat(const QString& userMessage);
    void reloadModel();

signals:
    void responseReceived(const QString& fullResponse);
    void responseChunk(const QString& chunk);
    void streamStarted();
    void streamFinished();
    void errorOccurred(const QString& error);
    void modelLoaded(bool ok);

private:
    QString    makeRequest(const QString& endpoint,
                        const QString& method,
                        const QString& payload = "");

    QString    makeStreamingRequest(const QString& endpoint,
                                 const QString& payload);

    QString    baseUrl()          const;
    QString    parseChatResponse(const QString& jsonResponse);
    bool       loadSystemPrompt();
    QJsonArray buildMessages(const QString& userMessage) const;
    void       setApiKey();

    // Generates a session-unique filename: "history_YYYYMMDD_HHMMSS_mmm.cbor"
    static QString makeSessionHistoryPath();

    // Shared CBOR read logic used by loadHistory(), loadSessionFromPath(),
    // and the static listSessions() — no instance state touched.
    static QJsonArray readCborHistory(const QString& path);

    QNetworkAccessManager* _nam = nullptr;

    QString    systemPrompt;
    QString    _apiKey          = "";
    QString    _serverIP        = "192.168.68.100";
    int        _serverPort      = 4321;
    QString    _modelName       = "gemma-4-26b-a4b-it";
    QString    sys_prompt_file  = "://Icons/sys_prompt.txt";

    QString    history_file_path;   // unique per instance, set in ctor

    QJsonArray _history;
    bool       model_loaded = false;

    std::string get_env_var(const std::string& key);
};

#endif // CWRAPPER_H
