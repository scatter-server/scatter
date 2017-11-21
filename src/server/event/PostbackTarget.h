/**
 * wsserver
 * PostbackTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_POSTBACKTARGET_H
#define WSSERVER_POSTBACKTARGET_H

#include "EventNotifier.h"
#include "EventConfig.h"

namespace wss {
namespace event {

class PostbackTarget : public WebTarget {
 public:
    explicit PostbackTarget(const json &config);
    bool send(const wss::MessagePayload &payload) override;
    string getType() override;

 private:
    std::string url;
};

}
}

#endif //WSSERVER_POSTBACKTARGET_H
