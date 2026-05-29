#include "HWpilotLLM.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QUrl>

HWpilotLLM::HWpilotLLM(const QString& apiKey, QObject* parent)
    : QObject(parent), m_apiKey(apiKey) {
    m_apiUrl = "https://api.deepseek.com/chat/completions";
    m_networkManager = new QNetworkAccessManager(this);

    // 连接网络管理器的 finished 信号到我们的槽函数
    connect(m_networkManager, &QNetworkAccessManager::finished, this,
            &HWpilotLLM::onNetworkReply);
}

void HWpilotLLM::sendChatRequest(const QJsonArray& messages,
                                 double temperature) {
    QNetworkRequest request((QUrl(m_apiUrl)));

    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",
                         QString("Bearer %1").arg(m_apiKey).toUtf8());

    // 构建 JSON 请求体
    QJsonObject requestBody;
    requestBody["model"] = "deepseek-chat";  // 使用 deepseek 聊天模型
    requestBody["messages"] = messages;
    requestBody["temperature"] = temperature;

    QJsonDocument doc(requestBody);
    QByteArray postData = doc.toJson();

    // 发送 POST 请求 (异步非阻塞)
    m_networkManager->post(request, postData);
}

void HWpilotLLM::onNetworkReply(QNetworkReply* reply) {
    const QByteArray responseData = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString responseBody = QString::fromUtf8(responseData).trimmed();

    if (reply->error() == QNetworkReply::NoError) {
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
                    reply->deleteLater();
                    return;
                }
            }
        }

        emit errorOccurred(QString("Unexpected API response. HTTP status: %1\nParse error: %2\nResponse body:\n%3")
                               .arg(statusCode)
                               .arg(parseError.errorString())
                               .arg(responseBody.left(4000)));
    } else {
        const QString detail = QString("Network/API error. HTTP status: %1\nQt error: %2\nResponse body:\n%3")
                                   .arg(statusCode)
                                   .arg(reply->errorString())
                                   .arg(responseBody.left(4000));
        emit errorOccurred(detail);
        qDebug().noquote() << detail;
    }

    reply->deleteLater();  // 释放内存
}
