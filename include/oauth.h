#ifndef OAUTH_H
#define OAUTH_H

#include "data_structures.h"
#include "handler.h"

extern Client default_test_client;

class OauthHandler : public Handler {
    std::map<QString, QString> formdata;
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    OauthHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // OAUTH_H
