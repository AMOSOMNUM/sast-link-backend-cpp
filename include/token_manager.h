#ifndef TOKEN_MANAGER_H
#define TOKEN_MANAGER_H

#include "handler.h"

#include <shared_mutex>
#include <QDateTime>
#include <QCryptographicHash>

#define OauthExpireTime 86400

class TokenManager {
    class Token {
        QString key;
        QDateTime create;
        //Unit: second
        int expire;
    public:
        Token() = default;
        Token (const QString& key, const QString& extra_info, QString& token, int expire = 10) : key(key), expire(expire) {
            create = QDateTime::currentDateTimeUtc();
            QString seed_with_time = key + extra_info + create.toString();
            token = QCryptographicHash::hash(seed_with_time.toUtf8(), QCryptographicHash::Md5).toHex();
        }

        void fetch(QString& result, Handler::Error& err) const {
            if (isExpired())
                err = Handler::Error(int(Handler::CommonErrCode::Unauthorized), "Token Expired!");
            else if (key.isNull())
                err = Handler::Error(int(Handler::CommonErrCode::Forbidden), "Token Invalid!");
            else
                result = std::move(key);
        }

        inline bool isExpired() const {
            return QDateTime::currentDateTimeUtc().secsTo(create) >= expire;
        }
    };

    std::shared_mutex lock;
    std::map<QString, Token> stored;
    std::map<QString, QString> users;

    TokenManager() = default;
public:
    static TokenManager& instance() {
        static TokenManager singleton;
        return singleton;
    }

    bool fetch(const QString& token, QString& result, Handler::Error& err) {
        std::shared_lock guard(lock);
        if (!stored.count(token)) {
            err = Handler::Error(int(Handler::CommonErrCode::Forbidden), "Token Invalid!");
            return false;
        }
        QString key;
        stored[token].fetch(key, err);
        if (users[key] != token)
            err = Handler::Error(int(Handler::CommonErrCode::Forbidden), "Token Invalid!");
        if (!err) {
            result = key;
            return true;
        }
        return false;
    }

    inline QString create(const QString& email, int expire = 10, QString extra_info = QString()) {
        std::lock_guard guard(lock);
        QString key = email.toLower();
        QString token;
        Token token_info(key, extra_info, token, expire);
        while (stored.count(token) && !stored[token].isExpired()) {
            extra_info += " ";
            token_info = Token(key, extra_info, token, expire);
        }
        stored[token] = token_info;
        users[key] = token;
        return token;
    }
};

#endif // TOKEN_MANAGER_H
