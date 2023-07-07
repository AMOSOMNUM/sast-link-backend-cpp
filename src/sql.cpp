#include "fake_sql.h"

FakeSelectQuery _SQL::select(QStringList fields) const {
    QList<QVariantList> result;
    for (const auto& i : user_table) {
        result.append(QVariantList());
        for (const auto& field : fields)
            result.back().append(abstract(i, field));
    }
    return FakeSelectQuery(std::move(fields), std::move(result));
}

void _SQL::alter(const QStringList& match_fields, const QVariantList& match_values, const QStringList& fields, const QVariantList& values, bool has_lock) {
    if (!has_lock)
        lock.lock();
    for (auto i = user_table.begin(); i != user_table.end();) {
        int index;
        for (index = 0; index < fields.size(); index++) {
            if (abstract(*i, fields[index]) != match_values[index])
                break;
        }
        if (index == fields.size())
            assign(*i, fields, values);
        else
            ++i;
    }
    if (!has_lock)
        lock.unlock();
}

void _SQL::remove(QStringList fields, QVariantList values, const bool has_lock) {
    if (!has_lock)
        lock.lock();
    for (auto i = user_table.begin(); i != user_table.end();) {
        int index;
        for (index = 0; index < fields.size(); index++) {
            if (abstract(*i, fields[index]) != values[index])
                break;
        }
        if (index == fields.size())
            i = user_table.erase(i);
        else
            ++i;
    }
    if (!has_lock)
        lock.unlock();
}
