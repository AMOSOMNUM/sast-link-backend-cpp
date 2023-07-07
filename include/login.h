#ifndef LOGIN_H
#define LOGIN_H

#include "handler.h"

#include <mutex>
#include <unordered_set>

class LoginStatus {
    std::mutex lock;
    std::unordered_set<QString> logged;
private:
    LoginStatus() = default;
public:
    static LoginStatus& instance() {
        static LoginStatus singleton;
        return singleton;
    }
public:
    void login(const QString& username) {}
    void logout(const QString& username);
};

class LoginHandler : public Handler{
private:
    virtual bool accept(Error& err) override;
    virtual Response process() override;
public:
    LoginHandler(const QHttpServerRequest& request) : Handler(request) {}
};

#endif // LOGIN_H
