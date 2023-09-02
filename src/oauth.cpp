#include "oauth.h"
#include "token_manager.h"

Client default_test_client = {
    .id = "114514",
    .secret = "1919810",
    .redirect_uri = "localhost:1919"
};

bool OauthHandler::accept(Error& err) {
    if (request.method() != QHttpServerRequest::Method::Post) {
        err = Error(int(CommonErrCode::Method_Not_Allowed), "Method Not Allowed");
        return false;
    }
    enum NAMES {
        ClientId,
        CodeChallenge,
        CodeChallengeMethod,
        RedirectURI,
        ResponseType,
        Scope,
        State,
    };
    bool check[7];
    memset(check, 0, sizeof(check));
    for (const auto& i : request.headers())
        if (i.first == "client_id") {
            if (i.second == default_test_client.id)
                check[ClientId] = true;
            break;
        }
        else if (i.first == "code_challenge") {
            if (i.second == "YillThSRrGTj6mXqFfDPinX7G35qEQ1QEyWV6PDSEuc%3D")
                check[CodeChallenge] = true;
            break;
        }
        else if (i.first == "code_challenge_method") {
            if (i.second == "S256")
                check[CodeChallengeMethod] = true;
            break;
        }
        else if (i.first == "redirect_uri") {
            if (i.second == default_test_client.redirect_uri)
                check[RedirectURI] = true;
            break;
        }
        else if (i.first == "response_type") {
            if (i.second == "code")
                check[ResponseType] = true;
            break;
        }
        else if (i.first == "scope") {
            if (i.second == "all")
                check[Scope] = true;
            break;
        }
        else if (i.first == "state") {
            if (i.second == "xyz")
                check[State] = true;
            break;
        }
    for (int i = 0; i < 7; i++)
        if (!check[i]){
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
    QString token = formdata["token"];
    Error err;
    QString username;
    if (!TokenManager::instance().fetch(token, username, err))
        return err.make_response();
    auto access_key = default_test_client.create_code(token);
}
