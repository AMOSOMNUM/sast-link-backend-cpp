#include "login.h"

#include "fake_sql.h"
#include "token_manager.h"

bool LoginHandler::accept() {
    if (request.method() != QHttpServerRequest::Method::Get) {
        onError(Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed"));
        return false;
    }
    bool pass = true;
    const auto& headers = request.headers();
    for (const auto& i : headers)
        if (i.first == "Authorization")
            if (i.second.trimmed().startsWith("Bearer")) {
                pass = true;
                break;
            }
    if (!pass || request.value("password").isNull() || request.value("password").isEmpty()) {
        onError(Error(int(CommonErrCode::Not_Found), "404 NOT FOUND"));
        return false;
    }
    return true;
}

Response LoginHandler::process() {
    QString password = request.value("password");
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "Authorization") {
            token = i.second.trimmed().remove(0, 6);//"Bearer"
            break;
        }
    QString username;
    if (!TokenManager::instance().isValid(token, &username))
        return Error(int(CommonErrCode::Not_Found), "404 NOT FOUND").make_response();
    //数据校验
    auto query = _SQL::instance().select(QStringList() << "email" << "is_deleted" << "password");
    while (query.next())
        if (query.value(0) == username)
            if (!query.value(1).toBool())
                if (query.value(2) == password)
                    break;
                else
                    return Error(int(Handler::CommonErrCode::Forbidden), "Password Unmatch!").make_response();
    QString new_token = TokenManager::instance().create(username);
    Response response(new_token);
    LoginStatus::instance().login(username);
    return response;
}
