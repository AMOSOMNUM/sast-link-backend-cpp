#ifndef RESPONSE_H
#define RESPONSE_H

#include "json_deserialise.hpp"

#include <QHttpServerResponse>

struct TokenData {
    QString type;
    QString token;
    TokenData(const QString& token, bool flag) : type(flag ? "login-ticket" : "register-ticket"), token(token) {}

    operator QJsonValue() {
        QJsonObject obj;
        obj.insert(type, token);
        return std::move(obj);
    }
};

struct Response {
    bool success;
    std::optional<QString> errcode;
    std::optional<QString> errmsg;
    QJsonValue data;

    Response(bool success = true) : success(true) {}
    explicit Response(int errCode, QString msg) : success(false), errcode(QString::number(errCode)), errmsg(msg) {}
    Response(const QString& token, bool flag) : success(true) {
        TokenData json(token, flag);
        data = json;
    }
    QHttpServerResponse toRequest() const;
};

register_object_member(Response, "Success", success);
register_object_member(Response, "ErrCode", errcode);
register_object_member(Response, "ErrMsg", errmsg);
register_object_member(Response, "Data", data);
declare_object(Response,
               object_member(Response, success),
               object_member(Response, errcode),
               object_member(Response, errmsg),
               object_member(Response, data)
               )

#endif // RESPONSE_H
