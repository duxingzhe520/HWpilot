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
    if (reply->error() == QNetworkReply::NoError) {
        // 读取返回的 JSON 数据
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

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
                }
            }
        }
    } else {
        // 发生错误，发射错误信号
        emit errorOccurred(reply->errorString());
        // 可以通过 reply->readAll() 获取具体的服务器报错信息
        qDebug() << "API Error details:" << reply->readAll();
    }

    reply->deleteLater();  // 释放内存
}