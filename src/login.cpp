#include "login.h"

#include "fake_sql.h"
#include "token_manager.h"

bool LoginHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Get) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    bool pass = false;
    for (const auto& i : request.headers())
        if (i.first == "LOGIN_TICKET") {
            pass = true;
            break;
        }
    const auto& form = request.query();
    if (!(pass && form.hasQueryItem("password"))) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response LoginHandler::process() {
    const auto& form = request.query();
    QString password = form.queryItemValue("password");
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "LOGIN_TICKET") {
            token = i.second;
            break;
        }
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();
    //数据校验
    _SQL::instance().getLock();
    auto query = _SQL::instance().select(QStringList() << "email" << "is_deleted" << "password");
    while (query.next())
        if (query.value(0) == username)
            if (!query.value(1).toBool())
                if (query.value(2) == password)
                    break;
                else
                    return Error(int(Handler::CommonErrCode::Forbidden), "Password Unmatch!").make_response();
    _SQL::instance().unlock();

    QString new_token = TokenManager::instance().create(username, OauthExpireTime);
    Response response(new_token, true);
    return response;
}
