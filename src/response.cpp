#include "response.h"

QHttpServerResponse Response::toRequest() const {
    declare_top_serialiser(*this, serialiser);
    QHttpServerResponse response(serialiser.to_json().toObject(), QHttpServerResponder::StatusCode::Ok);
    return response;
}
