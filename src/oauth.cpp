#include "oauth.h"
#include "token_manager.h"

Client default_test_client = {
    "114514",//id
    "1919810",//secret
    "http://localhost:1919"//redirect_uri
};

bool OauthHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    const auto& query = request.query();
    if (!(query.hasQueryItem("client_id") && query.queryItemValue("client_id") == default_test_client.id)
    ||  !(query.hasQueryItem("code_challenge") && query.queryItemValue("code_challenge") == "YillThSRrGTj6mXqFfDPinX7G35qEQ1QEyWV6PDSEuc=")
    ||  !(query.hasQueryItem("code_challenge_method") && query.queryItemValue("code_challenge_method") == "S256")
    ||  !(query.hasQueryItem("redirect_uri") && query.queryItemValue("redirect_uri") == default_test_client.redirect_uri)
    ||  !(query.hasQueryItem("response_type") && query.queryItemValue("response_type") == "code")
    ||  !(query.hasQueryItem("scope") && query.queryItemValue("scope") == "all")
    ||  !(query.hasQueryItem("state") && query.queryItemValue("state") == "xyz")) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    decode(request, formdata);
    if (!(formdata.count("token"))) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response OauthHandler::process() {
    const QString& token = formdata["token"];
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();
    formdata["id"] = username;
    return Response();
}

QHttpServerResponse OauthHandler::redirect() {
    Response response = process();
    if (!response.success)
        return response.toResponse();
    auto token = default_test_client.create(formdata["token"], formdata["id"]);
    QHttpServerResponse result(QHttpServerResponse::StatusCode::Found);
    result.setHeader("Location", (default_test_client.redirect_uri + "?token=" + token).toUtf8());
    return result;
}

QString Client::create(const QString& code, const QString& uid) {
    lock.lock();
    auto seed = id + secret + code;
    auto hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Md5).toHex();
    auto token = hash.left(16);
    auto current_time = QDateTime::currentDateTimeUtc();
    while (access_tokens.count(token) && (access_tokens[token].first >= current_time || storage[access_tokens[token].second] != uid)) {
        seed.append(' ');
        hash = QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Md5).toHex();
        token = hash.left(16);
    }
    if (access_tokens.count(token) && access_tokens[token].first < current_time) {
        auto access = access_tokens[token].second;
        auto uid = storage[access];
        users.erase(uid);
        storage.erase(access);
    }
    if (users.count(uid)) {
        auto token = users[uid];
        auto access = access_tokens[token].second;
        access_tokens.erase(token);
        storage.erase(access);
    }
    auto access = hash.right(16);
    access_tokens[token] = std::make_pair(current_time, access);
    storage[access] = uid;
    users[uid] = token;
    lock.unlock();
    return token;
}

bool AccessTokenHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    const auto& query = request.query();
    if (!(query.hasQueryItem("client_id") && query.queryItemValue("client_id") == default_test_client.id)
    ||  !(query.hasQueryItem("client_secret") && query.queryItemValue("client_secret") == default_test_client.secret)
    ||  !query.hasQueryItem("code")
    ||  !(query.hasQueryItem("code_verifier") && query.queryItemValue("code_verifier") == "sast_forever")
    ||  !(query.hasQueryItem("grant_type") && query.queryItemValue("grant_type") == "authorization_code")) {
        err = Error(int(CommonErrCode::Not_Found), "404 NOT FOUND");
        return false;
    }
    return true;
}

Response AccessTokenHandler::process() {
    QString code;
    for (const auto& i : request.headers())
        if (i.first == "code") {
            code = i.second;
            break;
        }
    return Response(AccessTokenData(default_test_client.get_access_token(code)));
}
