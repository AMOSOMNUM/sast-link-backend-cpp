#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "json_deserialise.hpp"

// SQL Only
struct Admin {
    QString user_id;

    bool dataLengthValid() {
        return user_id.length() < 256;
    }

    Admin() = default;
    Admin(const QJsonObject &json) {
        declare_deserialiser("user_id", user_id, uid_serialiser);

        JsonDeserialise::JsonDeserialiser seriliser(uid_serialiser);
        seriliser.deserialise(json);
    }

    QJsonValue to_json() {
        declare_deserialiser("user_id", user_id, uid_serialiser);

        JsonDeserialise::JsonDeserialiser seriliser(uid_serialiser);
        return seriliser.serialise_to_json();
    }
};

declare_class_with_json_constructor_and_serialiser(Admin);

struct Contact {
    QString *qq_id = nullptr;
    QString *github_id = nullptr;
    QString *lark_id = nullptr;
    QString *wechat_id = nullptr;

    bool dataLengthValid() {
        return (qq_id ? qq_id->length() <= 12 : true) &&
               (github_id ? github_id->length() <= 50 : true) &&
               (lark_id ? lark_id->length() <= 50 : true) &&
               (wechat_id ? wechat_id->length() <= 50 : true);
    }
    ~Contact() {
        delete qq_id;
        delete github_id;
        delete lark_id;
        delete wechat_id;
    }
};


// Json & SQL
struct User {
    QString email;
    QString uid;
    bool is_deleted = false;
    QString password;
    Contact contact;

    bool dataLengthValid() {
        return email.length() <= 30 && password.length() <= 30 && password.length() >= 6 && contact.dataLengthValid();
    }

    inline operator bool() {
        return !is_deleted;
    }

    inline void set_uid(unsigned num) {
        uid = QString::number(num);
    }

    User() = default;
    User(const QJsonObject &json) {
        declare_deserialiser("id", uid, id_serialiser);
        declare_deserialiser("email", email, email_serialiser);
        declare_deserialiser("password", password, pwd_serialiser);
        declare_deserialiser("QQ_id", contact.qq_id, qq_serialiser);
        declare_deserialiser("Github_id", contact.github_id, github_serialiser);
        declare_deserialiser("Lark_id", contact.lark_id, lark_serialiser);
        declare_deserialiser("Wechat_id", contact.lark_id, wechat_serialiser);

        JsonDeserialise::JsonDeserialiser seriliser(
            id_serialiser,
            email_serialiser,
            pwd_serialiser,
            qq_serialiser,
            github_serialiser,
            lark_serialiser,
            wechat_serialiser);
        seriliser.deserialise(json);
    }

    QJsonValue to_json() {
        declare_deserialiser("id", uid, id_serialiser);
        declare_deserialiser("email", email, email_serialiser);
        declare_deserialiser("password", password, pwd_serialiser);
        declare_deserialiser("QQ_id", contact.qq_id, qq_serialiser);
        declare_deserialiser("Github_id", contact.github_id, github_serialiser);
        declare_deserialiser("Lark_id", contact.lark_id, lark_serialiser);
        declare_deserialiser("Wechat_id", contact.lark_id, wechat_serialiser);

        JsonDeserialise::JsonDeserialiser seriliser(
            id_serialiser,
            email_serialiser,
            pwd_serialiser,
            qq_serialiser,
            github_serialiser,
            lark_serialiser,
            wechat_serialiser);
        return seriliser.serialise_to_json();
    }
};

declare_class_with_json_constructor_and_serialiser(User);

struct Client {
    QString id, secret, redirect_uri;
    //std::map<QString, QString> refresh_tokens;
    std::map<QString, QString> access_tokens;

    QString create_code(const QString& token);
    QString get_access_code(const QString& code);
};

#endif // DATA_STRUCTURES_H
