#ifndef SERVER_H
#define SERVER_H

#include "account_exist.h"
#include "fake_sql.h"
#include "login.h"
#include "logout.h"
#include "mail.h"
#include "oauth.h"
#include "thread_pool.h"
#include "register.h"

#include <QHttpServer>
#include <QTimer>

class Server : public QObject{
    QHttpServer server;
    QTimer* timer;
public:
    Server() {
        server.route("/api/v1/user/register", [](const QHttpServerRequest& request) {
            RegisterHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/verify/account", [](const QHttpServerRequest& request) {
            AccountExistHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/sendEmail", [](const QHttpServerRequest& request) {
            SendMailHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/verify/captcha", [](const QHttpServerRequest& request) {
            VerifyMailHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/user/login", [](const QHttpServerRequest& request) {
            LoginHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/user/logout", [](const QHttpServerRequest& request) {
            LogoutHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });
        server.route("/api/v1/oauth2/authorize", [](const QHttpServerRequest& request) {
            OauthHandler handler(request);
            Future<QHttpServerResponse> future([&](){return handler();});
            future.wait();
            return future.move();
        });
        server.route("/api/v1/oauth2/token", [](const QHttpServerRequest& request) {
            AccessTokenHandler handler(request);
            Future<QJsonObject> future([&](){return handler();});
            future.wait();
            return future.get();
        });


        server.listen(QHostAddress::LocalHost, 8080);
        timer = new QTimer;
        connect(timer, &QTimer::timeout, [](){
            _SQL::instance().save();
            qDebug() << QDateTime::currentDateTime().toString() << "-saved.";
        });
        timer->setInterval(50000);
        timer->start();
    }
    ~Server() {
        delete timer;
    }
};

#endif // SERVER_H
