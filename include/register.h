#ifndef REGISTER_H
#define REGISTER_H

#include "handler.h"

#include <mutex>

class RegisterHandler : public Handler {
    std::map<QString, QString> formdata;
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    RegisterHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // REGISTER_H
