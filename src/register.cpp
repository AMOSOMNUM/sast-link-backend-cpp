#include "register.h"
#include "fake_sql.h"
#include "token_manager.h"

bool RegisterHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    bool pass = true;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER_TICKET") {
            pass = true;
            break;
        }
    const auto& form = Handler::decode(request.body());
    if (!(pass && form.count("password"))) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response RegisterHandler::process() {
    //token校验
    QString token;
    for (const auto& i : request.headers())
        if (i.first == "REGISTER_TICKET") {
            token = i.second;
            break;
        }
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();

    auto form = Handler::decode(request.body());
    QString password = form["password"];

    //数据校验
    User data;
    data.email = username;
    data.password = password;
    if (!data.dataLengthValid()) {
        _SQL::instance().unlock();
        err = Error(int(Handler::CommonErrCode::Forbidden), "Too Short Or Too Long!");
        return err.make_response();
    }
    //数据写入
    _SQL::instance().insert(std::move(data));

    QString new_token = TokenManager::instance().create(username);
    return Response (new_token, false);
}
