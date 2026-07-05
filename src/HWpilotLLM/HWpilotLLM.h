#ifndef HWPILOTLLM_H
#define HWPILOTLLM_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QProcess>

class HWpilotLLM : public QObject {
    Q_OBJECT
   public:
    explicit HWpilotLLM(const QString& apiKey, QObject* parent = nullptr);

    // 发送带有历史记录的多轮对话请求
    // temperature: 创新度 (0.0~1.0)
    void sendChatRequest(const QJsonArray& messages, double temperature = 0.7);

   signals:
    // 当AI回复成功时触发该信号
    void responseReceived(const QString& replyText);

    // 当网络或API报错时触发该信号
    void errorOccurred(const QString& errorString);

   private slots:
    // 处理网络请求返回的槽函数
    void onNetworkReply(QNetworkReply* reply);

   private:
    bool handleApiResponse(const QByteArray& responseData,
                           const QString& statusText);
    void sendCurlFallback(const QByteArray& postData,
                          const QString& originalError);

    QString m_apiKey;
    QString m_apiUrl;
    QNetworkAccessManager* m_networkManager;
};

#endif  // HWPILOTLLM_H
