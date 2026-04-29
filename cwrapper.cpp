#include "cwrapper.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCborValue>
#include <QCborArray>
#include <QCborMap>

// ── Construction / destruction ────────────────────────────────────────────────

CWrapper::CWrapper(QObject *parent)
    : QObject{parent}
    , history_file_path(makeSessionHistoryPath())
{}

CWrapper::~CWrapper()
{
    // Never write a file for sessions that had no conversation.
    // This prevents empty ghost files from appearing in the history panel.
    if (!_history.isEmpty())
        saveHistory();
}

void CWrapper::init()
{
    _nam = new QNetworkAccessManager(this);
    loadSystemPrompt();

    // Only load from disk if a session wasn't pre-loaded by loadSessionFromPath().
    // If _history is already populated we just keep it; also keep the adopted
    // history_file_path so saves go to the right file.
    if (_history.isEmpty())
        loadHistory();

    setApiKey();
    bool ok = loadModel();
    emit modelLoaded(ok);
}

/* ── helpers ─────────────────────────────────────────────────────────────── */

QString CWrapper::makeSessionHistoryPath()
{
    return QStringLiteral("history_")
    + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))
        + QStringLiteral(".cbor");
}

QString CWrapper::baseUrl() const
{
    return QString("http://%1:%2").arg(_serverIP).arg(_serverPort);
}

bool CWrapper::loadSystemPrompt()
{
    QFile f(sys_prompt_file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    systemPrompt = QString::fromUtf8(f.readAll()).trimmed();
    return true;
}

QJsonArray CWrapper::buildMessages(const QString& userMessage) const
{
    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        QJsonObject sys;
        sys["role"] = "system"; sys["content"] = systemPrompt;
        messages.append(sys);
    }
    for (const QJsonValue& v : _history) messages.append(v);
    QJsonObject user;
    user["role"] = "user"; user["content"] = userMessage;
    messages.append(user);
    return messages;
}

void CWrapper::setApiKey()
{
    if (!_apiKey.isEmpty()) return;
    QString key = QString::fromStdString(get_env_var("LMSTUDIO_API_KEY"));
    if (!key.isEmpty())
        _apiKey = key;
}

std::string CWrapper::get_env_var(const std::string& key)
{
    const char* val = std::getenv(key.c_str());
    if (val == nullptr) {
        qWarning() << "Environment variable '" + key + "' is not set.";
        return "";
    }
    return std::string(val);
}

QString CWrapper::parseChatResponse(const QString& json)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred("JSON parse error: " + err.errorString()); return {};
    }
    QJsonArray choices = doc.object()["choices"].toArray();
    if (choices.isEmpty()) { emit errorOccurred("No choices in response"); return {}; }
    return choices[0].toObject()["message"].toObject()["content"].toString();
}

/* ── core HTTP (blocking, non-streaming) ──────────────────────────────────── */

QString CWrapper::makeRequest(const QString& endpoint,
                              const QString& method,
                              const QString& payload)
{
    if (!_nam) return "Error: not initialised";
    if (!model_loaded && endpoint != "/v1/models") return "Error: model not loaded";

    QNetworkRequest req;
    req.setUrl(QUrl(baseUrl() + endpoint));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!_apiKey.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + _apiKey).toUtf8());

    QNetworkReply* reply = nullptr;
    if      (method == "POST")   reply = _nam->post(req, payload.toUtf8());
    else if (method == "GET")    reply = _nam->get(req);
    else if (method == "DELETE") reply = _nam->deleteResource(req);
    else return "Error: unsupported method";

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString e = "Error: " + reply->errorString();
        emit errorOccurred(e); reply->deleteLater(); return e;
    }
    QString r = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    return r;
}

/* ── streaming POST ──────────────────────────────────────────────────────── */

QString CWrapper::makeStreamingRequest(const QString& endpoint,
                                       const QString& payload)
{
    if (!_nam) return "Error: not initialised";

    QNetworkRequest req;
    req.setUrl(QUrl(baseUrl() + endpoint));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!_apiKey.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + _apiKey).toUtf8());

    QNetworkReply* reply = _nam->post(req, payload.toUtf8());

    QString fullText;
    QEventLoop loop;

    connect(reply, &QNetworkReply::readyRead, this, [&]() {
        while (reply->canReadLine()) {
            QString line = QString::fromUtf8(reply->readLine()).trimmed();
            if (!line.startsWith("data:")) continue;
            QString jsonStr = line.mid(5).trimmed();
            if (jsonStr == "[DONE]") continue;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
            if (err.error != QJsonParseError::NoError) continue;

            const QJsonArray  choices     = doc.object()["choices"].toArray();
            if (choices.isEmpty()) continue;
            const QJsonObject firstChoice = choices.first().toObject();
            const QString     delta       = firstChoice["delta"].toObject()["content"].toString();

            if (!delta.isEmpty()) {
                fullText += delta;
                emit responseChunk(delta);
            }
        }
    });

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString e = "Error: " + reply->errorString();
        emit errorOccurred(e); reply->deleteLater(); return e;
    }

    const QByteArray remaining = reply->readAll();
    for (const QByteArray& rawLine : remaining.split('\n')) {
        QString line = QString::fromUtf8(rawLine).trimmed();
        if (!line.startsWith("data:")) continue;
        QString jsonStr = line.mid(5).trimmed();
        if (jsonStr == "[DONE]") continue;

        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        const QJsonArray  choices     = doc.object()["choices"].toArray();
        if (choices.isEmpty()) continue;
        const QJsonObject firstChoice = choices.first().toObject();
        const QString     delta       = firstChoice["delta"].toObject()["content"].toString();

        if (!delta.isEmpty()) { fullText += delta; emit responseChunk(delta); }
    }

    reply->deleteLater();
    return fullText;
}

/* ── public API ──────────────────────────────────────────────────────────── */

QString CWrapper::listModels()
{
    bool s = model_loaded; model_loaded = true;
    QString r = makeRequest("/v1/models", "GET");
    model_loaded = s; return r;
}

bool CWrapper::loadModel()
{
    bool s = model_loaded; model_loaded = true;
    QString json = makeRequest("/v1/models", "GET");
    model_loaded = s;

    if (json.startsWith("Error:")) {
        qWarning() << "loadModel:" << json; model_loaded = false; return false;
    }
    QJsonArray data = QJsonDocument::fromJson(json.toUtf8()).object()["data"].toArray();
    for (const QJsonValue& v : std::as_const(data)) {
        if (v.toObject()["id"].toString() == _modelName) {
            model_loaded = true; return true;
        }
    }
    if (!data.isEmpty()) {
        _modelName   = data.first().toObject()["id"].toString();
        model_loaded = true;
        qDebug() << "CWrapper: using model" << _modelName;
        return true;
    }
    model_loaded = false;
    return false;
}

void CWrapper::reloadModel()
{
    bool ok = loadModel();
    emit modelLoaded(ok);
}

void CWrapper::chat(const QString& userMessage)
{
    if (!model_loaded) { emit errorOccurred("Model not loaded"); return; }

    QJsonObject body;
    body["model"]    = _modelName;
    body["messages"] = buildMessages(userMessage);
    body["stream"]   = true;

    emit streamStarted();
    QString full = makeStreamingRequest("/v1/chat/completions",
                                        QJsonDocument(body).toJson(QJsonDocument::Compact));
    emit streamFinished();

    if (full.startsWith("Error:")) { emit errorOccurred(full); return; }

    QJsonObject u; u["role"] = "user";      u["content"] = userMessage;
    QJsonObject a; a["role"] = "assistant"; a["content"] = full;
    _history.append(u); _history.append(a);

    emit responseReceived(full);
}

void CWrapper::clearHistory() { _history = QJsonArray(); }

/* ── CBOR persistence ─────────────────────────────────────────────────────── */

QJsonArray CWrapper::readCborHistory(const QString& path)
{
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return {};

    const QCborValue root = QCborValue::fromCbor(f.readAll());
    if (!root.isArray()) return {};

    QJsonArray result;
    for (const QCborValue& entry : root.toArray()) {
        if (!entry.isMap()) continue;
        const QCborMap m = entry.toMap();
        QJsonObject obj;
        obj["role"]    = m[QStringLiteral("role")].toString();
        obj["content"] = m[QStringLiteral("content")].toString();
        result.append(obj);
    }
    return result;
}

bool CWrapper::saveHistory()
{
    // Caller should already guard against empty history, but be defensive.
    if (_history.isEmpty()) return true;

    QCborArray cborHistory;
    for (const QJsonValue& v : std::as_const(_history)) {
        const QJsonObject obj = v.toObject();
        QCborMap entry;
        entry[QStringLiteral("role")]    = obj["role"].toString();
        entry[QStringLiteral("content")] = obj["content"].toString();
        cborHistory.append(entry);
    }

    QFile f(history_file_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QCborValue(cborHistory).toCbor());
    return true;
}

bool CWrapper::loadHistory()
{
    _history = readCborHistory(history_file_path);
    return !_history.isEmpty();
}

bool CWrapper::loadSessionFromPath(const QString& path)
{
    QJsonArray loaded = readCborHistory(path);
    if (loaded.isEmpty()) return false;
    history_file_path = path;   // saves will update this file
    _history = std::move(loaded);
    return true;
}

/* ── static: list past sessions ──────────────────────────────────────────── */

QList<SessionInfo> CWrapper::listSessions(int n)
{
    const QFileInfoList files =
        QDir(QStringLiteral("."))
            .entryInfoList(QStringList() << QStringLiteral("history_*.cbor"),
                           QDir::Files, QDir::Name | QDir::Reversed);

    QList<SessionInfo> result;
    result.reserve(files.size());

    for (const QFileInfo& fi : files) {
        // Only list sessions that actually have content
        const QJsonArray history = readCborHistory(fi.absoluteFilePath());
        if (history.isEmpty()) continue;  // skip empty / corrupted files

        const QString stem  = fi.baseName();   // "history_20250428_143502_042"
        const QString dtStr = stem.mid(8);     // "20250428_143502_042"
        QDateTime ts = QDateTime::fromString(dtStr, QStringLiteral("yyyyMMdd_HHmmss_zzz"));
        if (!ts.isValid()) ts = fi.lastModified();

        QString preview;
        for (const QJsonValue& v : history) {
            const QJsonObject msg = v.toObject();
            if (msg["role"].toString() == QLatin1String("user")) {
                preview = msg["content"].toString();
                break;
            }
        }
        if (preview.isEmpty()) continue;  // no user turn — skip

        result.append(SessionInfo{ fi.absoluteFilePath(), ts, preview });

        if (n > 0 && result.size() >= n)
            break;
    }

    return result;
}
