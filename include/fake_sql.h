#ifndef FAKE_SQL_H
#define FAKE_SQL_H

#include "data_structures.h"

#include <shared_mutex>

struct FakeSelectQuery {
    QStringList fields;
    QList<QVariantList> result;
    QList<QVariantList>::Iterator current;
    bool started = false;

    inline bool next() {
        if (!started)
            started = true;
        else
            ++current;
        return current != result.end();
    }

    inline QVariant value(std::size_t index) {
        return current->value(index);
    }

    FakeSelectQuery(QStringList&& fields_, QList<QVariantList>&& result_) : fields(std::move(fields_)), result(std::move(result_)), current(result.begin()) {}
    FakeSelectQuery(FakeSelectQuery&& other) : FakeSelectQuery(std::move(other.fields), std::move(other.result)) {}
};

template <typename T, typename = JsonDeserialise::DeserialisableType<T>>
using _Table = std::vector<T>;

class _SQL {
    mutable std::shared_mutex lock;
    _Table<User> user_table;
    _Table<Admin> admin_table;
public:
    void alter(const QStringList& match_fields, const QVariantList& match_values, const QStringList& fields, const QVariantList& values, bool has_lock = false);
    FakeSelectQuery select(QStringList fields) const;
    void insert(User&& value, const bool has_lock = false) {
        if (!has_lock)
            lock.lock();
        value.set_uid(user_table.empty() ? 1 : user_table.back().uid.toInt() + 1);
        user_table.emplace_back(value);
        if (!has_lock)
            lock.unlock();
    }
    void remove(QStringList fields, QVariantList values, const bool has_lock = false);
    inline void getLock() {
        lock.lock();
    }
    inline void unlock() {
        lock.unlock();
    }
    inline void getReadLock() {
        lock.lock();
    }
    inline void unlockRead() {
        lock.unlock();
    }

    static _SQL& instance() {
        static _SQL singleton;
        return singleton;
    }
    void save() {
        lock.lock();
        declare_serialiser("User", user_table, user_);
        declare_serialiser("Admin", admin_table, admin_);
        JsonDeserialise::JsonSerialiser serialiser(user_, admin_);
        serialiser.serialise_to_file("data.json");
        lock.unlock();
    }
private:
    inline QVariant abstract(const User& data, const QString& index) const {
        if (index == "id")
            return data.uid;
        else if (index == "email")
            return data.email;
        else if (index == "password")
            return data.password;
        else if (index == "is_deleted")
            return data.is_deleted;
        else if (index == "QQ_id")
            return data.contact.qq_id ? QVariant(*data.contact.qq_id) : QVariant();
        else if (index == "Github_id")
            return data.contact.github_id ? QVariant(*data.contact.github_id) : QVariant();
        else if (index == "Lark_id")
            return data.contact.lark_id ? QVariant(*data.contact.lark_id) : QVariant();
        else if (index == "Wechat_id")
            return data.contact.wechat_id ? QVariant(*data.contact.wechat_id) : QVariant();
        throw std::ios_base::failure("Unknown Field!");
    }

    void assign(User& usr, const QStringList& fields, const QVariantList& values) {
        auto data = values.begin();
        for (auto field = fields.begin(); field != fields.end(); ++field, ++data)
            if (*field == "id")
                usr.uid = data->toString();
            else if (*field == "email")
                usr.email = data->toString();
            else if (*field == "password")
                usr.password = data->toString();
            else if (*field == "is_deleted")
                usr.is_deleted = data->toBool();
            else if (*field == "QQ_id")
                usr.contact.qq_id = new QString(data->toString());
            else if (*field == "Github_id")
                usr.contact.github_id = new QString(data->toString());
            else if (*field == "Lark_id")
                usr.contact.lark_id = new QString(data->toString());
            else if (*field == "Wechat_id")
                usr.contact.wechat_id = new QString(data->toString());
    }

    _SQL() {
        if (QFile("data.json").exists()) {
            declare_deserialiser("User", user_table, user_);
            declare_deserialiser("Admin", admin_table, admin_);
            JsonDeserialise::JsonDeserialiser deserialiser(user_, admin_);
            deserialiser.deserialiseFile("data.json");
        }
    }
};

#endif // FAKE_SQL_H
