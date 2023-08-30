#ifndef LOGIN_H
#define LOGIN_H

#include "handler.h"

class LoginHandler : public Handler {
    std::map<QString, QString> formdata;
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    LoginHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // LOGIN_H
