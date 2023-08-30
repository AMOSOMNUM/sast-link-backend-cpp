#include "mail.h"

#include "json_deserialise.hpp"
#include "token_manager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QSslSocket>

Mail::Mail() {
    declare_deserialiser("E-Mail", address, addr);
    declare_deserialiser("PWD", password, pwd);
    declare_deserialiser("Domain", server, ip);
    JsonDeserialise::JsonDeserialiser deserialiser(addr, pwd, ip);
    deserialiser.deserialiseFile(":content/settings.json");
}

QByteArray Mail::generateEmail(const QString& receiver, Handler::Error& err) {
    QByteArray result;
    QString seed_with_time = receiver.chopped(8) + QDateTime::currentDateTimeUtc().toString();
    auto encrypted = QCryptographicHash::hash(seed_with_time.toUtf8(), QCryptographicHash::Md5).toHex();
    QFile email_content_file(":content/captcha.html");

    email_content_file.open(QFile::ReadOnly);
    result = email_content_file.readAll();

    auto errCode = email_content_file.error();
    if (errCode == QFile::NoError) {
        QByteArray captcha;
        bool flag = 1;
        char c = '\0';
        for (auto i : encrypted) {
            static char charset[] = "ABC9DEF8GHJ7KLM6NPQ5RST4UVW3XYZ";
            if (i >= 'a')
                c += i - 'a' + 10;
            else
                c += i - '0';
            flag = !flag;
            if (flag) {
                captcha.append(charset[c]);
                c = '\0';
            }
            if (captcha.length() >= 8)
                break;
        }
        result.replace("CAPTCHA", captcha);
        storeCaptcha(receiver, captcha, err);
        if (!err)
            return result;
    }
    else
        err = Handler::Error(600 + errCode, email_content_file.errorString());
}

void Mail::sendEmail(const QByteArray& receiver, Handler::Error& err) {
    auto data = generateEmail(receiver, err);
    if (err)
        return;
    QSslSocket socket;
    QByteArray response_code;
    //Connect
    socket.connectToHostEncrypted(server, 465);
    socket.waitForEncrypted(1000);
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("220")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }
    //HELO
    socket.write("HELO " + address + "\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("250")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }
    //AUTH
    socket.write("AUTH LOGIN\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("334")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }
    socket.write(address.toBase64() + "\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("334")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }
    socket.write(password.toBase64() + "\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("235")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }

    //MAIL FROM
    socket.write("MAIL FROM:<" + address + ">\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("250")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }

    //RCPT T0
    socket.write("RCPT TO:<" + receiver + ">\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("250")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }

    //DATA
    socket.write("DATA\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("354")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }

    //Body
    socket.write("From:" + address + "\r\n");
    socket.write("To:" + receiver + "\r\n");
    socket.write("Subject:SAST Link CAPTCHA\r\n");
    socket.write("MIME-Version:1.0\r\n");
    socket.write("Content-Type:text/html; charset=utf-8\r\n");
    socket.write("Content-Transfer-Encoding:base64\r\n");
    socket.write("\r\n");
    socket.write(data.toBase64());
    socket.write("\r\n.\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("250")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }

    //QUIT
    socket.write("QUIT\r\n");
    socket.waitForReadyRead(1000);
    response_code = socket.readAll();
    if (!response_code.startsWith("221")) {
        err = Handler::Error(int(MailSendErr::SendingFailed), "Mail Sending Failed!");
        return;
    }
    socket.close();
}

void Mail::storeCaptcha(const QString& email, QString&& captcha, Handler::Error& err, QDateTime now) {
    std::lock_guard guard(lock);
    if (stored.count(email))
        if (stored[email].first.secsTo(now) < MIN_INTERVAL) {
            err = Handler::Error(int(MailSendErr::TooFrequent), "Too Frequent Sending!");
            return;
        }
    stored[email] = std::make_pair(now, std::move(captcha));
}

bool Mail::verifyCaptcha(const QString& email, const QString& captcha, Handler::Error& err, QDateTime now) {
    std::lock_guard guard(lock);
    if (!stored.count(email)) {
        err = Handler::Error(int(MailSendErr::InvalidRequest), "Invalid Captcha Request!");
        return false;
    }
    if (stored[email].first.secsTo(now) > SURVIVAL_TIME) {
        err = Handler::Error(int(MailSendErr::CaptchaExpired), "Captcha Expired!");
        return false;
    }
    if (stored[email].second != captcha) {
        err = Handler::Error(int(MailSendErr::WrongCaptcha), "Captcha Unmatched!");
        return false;
    }
    return true;
}

bool SendMailHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Get) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    bool pass = true;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER-TICKET") {
            pass = true;
            break;
        }
    if (!pass) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

std::mutex SendMailHandler::lock;
std::list<std::pair<QString, QDateTime>> SendMailHandler::stored;

Response SendMailHandler::process() {
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER-TICKET") {
            token = i.second;
            break;
        }
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();
    //防止频繁触发
    {
        std::lock_guard guard(lock);
        for (auto i = stored.begin(); i != stored.end();)
            if (i->second.secsTo(QDateTime::currentDateTimeUtc()) > 60)
                i = stored.erase(i);
            else {
                if (i->first == username)
                    return Handler::Error(int(Mail::MailSendErr::TooFrequent), "Too Frequent Sending!").make_response();
                ++i;
            }
        stored.emplace_back(username, QDateTime::currentDateTimeUtc());
    }

    Mail::instance().sendEmail(username.toUtf8(), err);

    return Response();
}

bool VerifyMailHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    bool pass = true;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER-TICKET") {
            pass = true;
            break;
        }

    formdata = Handler::decode_url_form(request.body());
    if (!(pass && formdata.count("captcha"))) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response VerifyMailHandler::process() {
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER-TICKET") {
            token = i.second;
            break;
        }
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();

    QString captcha = formdata["captcha"].toUpper();
    if (!Mail::instance().verifyCaptcha(username, captcha, err))
        return err.make_response();

    return Response();
}
