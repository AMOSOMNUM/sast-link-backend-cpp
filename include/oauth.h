#ifndef OAUTH_H
#define OAUTH_H

#include "data_structures.h"
#include "handler.h"

extern Client default_test_client;

class OauthHandler : public RedirectHandler {
    std::map<QString, QString> formdata;
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
    virtual QHttpServerResponse redirect() override;
public:
    OauthHandler(const QHttpServerRequest& request) : RedirectHandler(request) {}
};

class AccessTokenHandler : public Handler {
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    AccessTokenHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // OAUTH_H
