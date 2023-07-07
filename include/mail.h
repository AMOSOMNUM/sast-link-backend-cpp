#ifndef MAIL_H
#define MAIL_H

#include "handler.h"

#include <mutex>
#include <QDateTime>

class Mail {
public:
    enum class MailSendErr : int {
        SendingFailed  = 626,
        TooFrequent    = 627,
        CaptchaExpired = 628,
        InvalidRequest = 629,
        WrongCaptcha   = 630
    };
private:
    //UNIT:second
    const static int MIN_INTERVAL = 30;
    const static int SURVIVAL_TIME = 600;
private:
    QByteArray address;
    QByteArray password;
    QString server;

    std::mutex lock;
    std::map<QString, std::pair<QDateTime, QString>> stored;
private:
    void storeCaptcha(const QString& email, QString&& captcha, Handler::Error& err, QDateTime now = QDateTime::currentDateTimeUtc());
    QByteArray generateEmail(const QString& receiver, Handler::Error& err);
public:
    bool verifyCaptcha(const QString& email, const QString& captcha, Handler::Error& err, QDateTime now = QDateTime::currentDateTimeUtc());
    void sendEmail(const QByteArray& receiver, Handler::Error& err);

    static Mail& instance() {
        static Mail singleton;
        return singleton;
    }
private:
    Mail();
};

class SendMailHandler : public Handler {
    static std::mutex lock;
    static std::list<std::pair<QString, QDateTime>> stored;
public:
    SendMailHandler(const QHttpServerRequest& request) : Handler(request) {}
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
};

class VerifyMailHandler : public Handler {
public:
    VerifyMailHandler(const QHttpServerRequest& request) : Handler(request) {}
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
};

#endif // MAIL_H
