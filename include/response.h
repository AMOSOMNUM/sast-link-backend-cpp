#ifndef RESPONSE_H
#define RESPONSE_H

#include "json_deserialise.hpp"

#include <QHttpServerResponse>

struct TokenData {
    QString type;
    QString token;

    TokenData(const QString& token, const QString& name) : type(name), token(token) {}
    TokenData(const QString& token, bool flag) : type(flag ? "login-ticket" : "register-ticket"), token(token) {}

    operator QJsonValue() {
        QJsonObject obj;
        obj.insert(type, token);
        return std::move(obj);
    }
};

struct AccessTokenData {
    QString access;
    int expire = 1800;
    //QString refresh;
    QString scope = "all";
    QString type = "Bearer";

    AccessTokenData(const QString& token) : access(token) {}
};

register_object_member(AccessTokenData, "access_token", access);
register_object_member(AccessTokenData, "expires_in", expire);
register_object_member(AccessTokenData, "scope", scope);
register_object_member(AccessTokenData, "token_type", type);
declare_object(AccessTokenData,
               object_member(AccessTokenData, access),
               object_member(AccessTokenData, expire),
               object_member(AccessTokenData, scope),
               object_member(AccessTokenData, type)
               )

struct Response {
    bool success;
    std::optional<QString> errcode;
    std::optional<QString> errmsg;
    QJsonValue data;

    explicit Response(bool success = true) : success(success) {}
    Response(int errCode, QString msg) : success(false), errcode(QString::number(errCode)), errmsg(msg) {}
    Response(const QString& token, bool flag) : success(true) {
        TokenData json(token, flag);
        data = json;
    }
    Response(const QString& token, const QString& key_name) : success(true) {
        TokenData json(token, key_name);
        data = json;
    }
    Response(const AccessTokenData& data) {
        declare_top_serialiser(data, holder);
        this->data = holder.to_json();
    }
    QHttpServerResponse toResponse() const;
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
