#ifndef HANDLER_H
#define HANDLER_H

#include "response.h"

#include <QHttpServerRequest>
#include <QHttpServerResponder>

class Handler {
public:
    enum class CommonErrCode : int {
        Unauthorized       = 401,
        Forbidden          = 403,
        Not_Found          = 404,
        Method_Not_Allowed = 405,
    };
    struct Error {
        int errcode = 0;
        QString errmsg;

        Error() = default;
        Error(int errCode, QString msg) : errcode(errCode), errmsg(msg) {}
        inline Response make_response() {
            return Response(errcode, errmsg);
        }
        inline operator bool() {
            return errcode;
        }
    };
    QJsonObject operator()() {
        Error err;
        auto response = accept(err) ? process() : err.make_response();
        declare_top_serialiser(response, serialiser);
        return serialiser.to_json().toObject();
    }
protected:
    const QHttpServerRequest& request;

    virtual bool accept(Error& err) = 0;
    virtual Response process() = 0;

    Handler() = delete;
    Handler(const QHttpServerRequest& request) : request(request) {}
    virtual ~Handler() {}

    static std::map<QString, QString> decode(const QByteArray& formdata) {
        std::map<QString, QString> result;
        const auto& form = formdata.split('&');
        for (const auto& i : form) {
            auto pair = i.split('=');
            result[pair[0]] = pair[1];
        }
        return std::move(result);
    }
};

#endif // HANDLER_H
