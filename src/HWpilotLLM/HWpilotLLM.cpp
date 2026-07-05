#include "HWpilotLLM.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QUrl>

namespace {
QString httpStatusText(QNetworkReply* reply) {
    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!status.isValid())
        return "n/a";
    return QString::number(status.toInt());
}
}  // namespace

HWpilotLLM::HWpilotLLM(const QString& apiKey, QObject* parent)
    : QObject(parent), m_apiKey(apiKey) {
    m_apiUrl = "https://api.deepseek.com/chat/completions";
    m_networkManager = new QNetworkAccessManager(this);

    qDebug() << "[HWpilotLLM] SSL supported:" << QSslSocket::supportsSsl()
             << "build:" << QSslSocket::sslLibraryBuildVersionString()
             << "runtime:" << QSslSocket::sslLibraryVersionString();

    // 连接网络管理器的 finished 信号到我们的槽函数
    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &HWpilotLLM::onNetworkReply);
}

void HWpilotLLM::sendChatRequest(const QJsonArray& messages,
                                 double temperature) {
    QNetworkRequest request((QUrl(m_apiUrl)));

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "HWpilot/1.0");
    request.setRawHeader("Authorization",
                         QString("Bearer %1").arg(m_apiKey).toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    // 构建 JSON 请求体
    QJsonObject requestBody;
    requestBody["model"] = "deepseek-chat";  // 使用 deepseek 聊天模型
    requestBody["messages"] = messages;
    requestBody["temperature"] = temperature;

    QJsonDocument doc(requestBody);
    QByteArray postData = doc.toJson();
    qDebug() << "[HWpilotLLM] POST" << m_apiUrl << "bytes:" << postData.size()
             << "messages:" << messages.size()
             << "temperature:" << temperature;

    // 发送 POST 请求 (异步非阻塞)
    QNetworkReply* reply = m_networkManager->post(request, postData);
    reply->setProperty("postData", postData);
    connect(reply, &QNetworkReply::sslErrors, this,
            [](const QList<QSslError>& errors) {
                for (const QSslError& error : errors)
                    qDebug() << "[HWpilotLLM] SSL error:" << error.errorString();
            });
}

void HWpilotLLM::onNetworkReply(QNetworkReply* reply) {
    const QByteArray responseData = reply->readAll();
    const QString responseText = QString::fromUtf8(responseData);
    const QString statusText = httpStatusText(reply);

    if (reply->error() == QNetworkReply::NoError) {
        handleApiResponse(responseData, statusText);
    } else {
        // 发生错误，发射错误信号
        const QString errorMessage = QString("Network/API error: %1. HTTP status: %2. Body: %3")
                                         .arg(reply->errorString(), statusText, responseText.left(4000));
        qDebug() << "[HWpilotLLM] error code:" << reply->error()
                 << "error:" << reply->errorString()
                 << "HTTP status:" << statusText
                 << "response bytes:" << responseData.size()
                 << "body:" << responseText.left(4000);

        if (reply->error() == QNetworkReply::RemoteHostClosedError &&
            statusText == "n/a") {
            const QByteArray postData = reply->property("postData").toByteArray();
            reply->deleteLater();
            sendCurlFallback(postData, errorMessage);
            return;
        }

        emit errorOccurred(errorMessage);
    }

    reply->deleteLater();  // 释放内存
}

bool HWpilotLLM::handleApiResponse(const QByteArray& responseData,
                                   const QString& statusText) {
    const QString responseText = QString::fromUtf8(responseData);

    // 读取返回的 JSON 数据
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();

        // 解析 DeepSeek (OpenAI 兼容) 的返回结构
        if (jsonObj.contains("choices") && jsonObj["choices"].isArray()) {
            QJsonArray choices = jsonObj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                QJsonObject message = firstChoice["message"].toObject();
                QString content = message["content"].toString();

                // 成功获取 AI 回复，发射信号！
                emit responseReceived(content);
                return true;
            }
        }
    }

    const QString errorMessage = QString("Unexpected API response. HTTP status: %1. JSON parse: %2. Body: %3")
                                     .arg(statusText, parseError.errorString(), responseText.left(2000));
    qDebug() << "[HWpilotLLM]" << errorMessage;
    emit errorOccurred(errorMessage);
    return false;
}

void HWpilotLLM::sendCurlFallback(const QByteArray& postData,
                                  const QString& originalError) {
    qDebug() << "[HWpilotLLM] Qt network request failed before HTTP response;"
             << "retrying with curl fallback.";

    QProcess* process = new QProcess(this);
    process->setProgram("curl");
    process->setArguments({
        "--silent",
        "--show-error",
        "--fail-with-body",
        "--location",
        "--request",
        "POST",
        m_apiUrl,
        "--header",
        "Content-Type: application/json",
        "--header",
        "Accept: application/json",
        "--header",
        QString("Authorization: Bearer %1").arg(m_apiKey),
        "--data-binary",
        "@-",
    });

    connect(process, &QProcess::started, process, [process, postData]() {
        process->write(postData);
        process->closeWriteChannel();
    });

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process, originalError](int exitCode,
                                           QProcess::ExitStatus exitStatus) {
                const QByteArray stdoutData = process->readAllStandardOutput();
                const QByteArray stderrData = process->readAllStandardError();

                qDebug() << "[HWpilotLLM] curl fallback finished."
                         << "exitCode:" << exitCode
                         << "exitStatus:" << exitStatus
                         << "stdout bytes:" << stdoutData.size()
                         << "stderr:" << QString::fromUtf8(stderrData).left(2000);

                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    handleApiResponse(stdoutData, "curl");
                } else {
                    const QString errorMessage =
                        QString("%1\nCurl fallback failed. Exit code: %2. Error: %3. Body: %4")
                            .arg(originalError)
                            .arg(exitCode)
                            .arg(QString::fromUtf8(stderrData).left(2000),
                                 QString::fromUtf8(stdoutData).left(2000));
                    emit errorOccurred(errorMessage);
                }

                process->deleteLater();
            });

    connect(process, &QProcess::errorOccurred, this,
            [this, process, originalError](QProcess::ProcessError error) {
                const QString errorMessage =
                    QString("%1\nCurl fallback could not start or run. QProcess error: %2. %3")
                        .arg(originalError)
                        .arg(error)
                        .arg(process->errorString());
                qDebug() << "[HWpilotLLM]" << errorMessage;
                emit errorOccurred(errorMessage);
                process->deleteLater();
            });

    process->start();
}
