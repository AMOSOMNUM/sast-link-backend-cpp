#include "account_exist.h"
#include "fake_sql.h"
#include "token_manager.h"

bool AccountExistHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Get) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    const auto& form = request.query();
    if (!(form.hasQueryItem("username") &&
        form.queryItemValue("username").toLower().endsWith("@njupt.edu.cn") &&
        form.hasQueryItem("flag") &&
        form.queryItemValue("flag") == "1" || form.queryItemValue("flag") == "0")) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response AccountExistHandler::process() {
    AccountExistInfo info;
    const auto& form = request.query();
    info.username = form.queryItemValue("username").toLower();
    info.flag = form.queryItemValue("flag").toUInt();
    //查询账号是否已注册
    _SQL::instance().getLock();
    bool registered = false;
    auto query = _SQL::instance().select(QStringList() << "email" << "is_deleted");
    while (query.next())
        if (query.value(0) == info.username) {
            if (registered = !query.value(1).toBool())
                break;
        }
    _SQL::instance().unlock();

    if (info.flag != registered)
        return Response(false);
    QString new_token = TokenManager::instance().create(info.username);
    return Response (new_token, info.flag);
}
