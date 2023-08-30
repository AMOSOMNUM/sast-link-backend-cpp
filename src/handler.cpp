#include "handler.h"

static QString read_key(const QByteArray& formdata, int& start, int& end) {
    end = formdata.indexOf("\r\n", start = end + 2);
    if (end == -1)
        return QString();
    auto firstLine = formdata.sliced(start, end - start);
    auto nameStart = firstLine.indexOf("Content-Disposition: form-data; name=\"", 0) + 38;
    if (nameStart == 37)
        return QString();
    auto nameEnd = firstLine.indexOf('\"', nameStart) - 1;
    if (nameEnd == -2)
        return QString();
    return firstLine.sliced(nameStart, nameEnd - nameStart + 1);
}

bool Handler::decode(const QHttpServerRequest& request, std::map<QString, QString>& result) {
    QByteArray bound;
    for (const auto& i : request.headers())
        if (i.first == "Content-Type") {
            if (!i.second.trimmed().startsWith("multipart/form-data"))
                return false;
            const auto& type = i.second;
            auto loc = type.indexOf("boundary");
            if (loc == -1)
                break;
            loc = type.indexOf('=', loc) + 1;
            bound = type.sliced(loc);
            break;
        }
    if (bound.isEmpty())
        return false;
    result.clear();
    const auto& formdata = request.body();
    if (!formdata.contains(bound))
        return false;
    QString key;
    QByteArray value;
    for (int last = 0, i = formdata.trimmed().indexOf("\r\n", last); i != -1; i = formdata.indexOf("\r\n", last = i + 2)) {
        const auto& line = formdata.sliced(last, i - last);
        if (line.contains(*bound)) {
            if (!key.isEmpty())
                result[QString(std::move(key))] = QString(std::move(value));
            key = read_key(formdata, last, i);
            if (i == -1)
                break;
            continue;
        }
        if (!line.trimmed().isEmpty())
            value.push_back(line);
    }
    return true;
}
