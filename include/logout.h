#ifndef LOGOUT_H
#define LOGOUT_H

#include "handler.h"

class LogoutHandler : public Handler{
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    LogoutHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // LOGOUT_H
