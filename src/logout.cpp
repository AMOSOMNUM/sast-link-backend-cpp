#include "logout.h"

#include "token_manager.h"

bool LogoutHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
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
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "TOKEN") {
            token = i.second;
            break;
        }
    Error err;
    if (!TokenManager::instance().remove(token, err))
        return err.make_response();
    return Response();
}
