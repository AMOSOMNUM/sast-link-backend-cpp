#include "logout.h"

#include "login.h"
#include "fake_sql.h"
#include "token_manager.h"

bool LogoutHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Get) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    bool pass = false;
    for (const auto& i : request.headers())
        if (i.first == "TOKEN") {
            pass = true;
            break;
        }
    if (!pass) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response LogoutHandler::process() {
    QString password = request.value("password");
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "TOKEN") {
            token = i.second;
            break;
        }    
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();
    LoginStatus::instance().logout(username);
    return Response();
}
