#ifndef ACCOUNT_EXIST_H
#define ACCOUNT_EXIST_H

#include "handler.h"

class AccountExistHandler : public Handler {
    struct AccountExistInfo {
        QString username;
        bool flag;  //true:Login,false:Register
    };
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    AccountExistHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // ACCOUNT_EXIST_H
