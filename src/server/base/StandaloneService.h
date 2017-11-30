/**
 * wsserver
 * joinable.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_JOINABLE_H
#define WSSERVER_JOINABLE_H

namespace wss {

class StandaloneService {
 public:
    virtual void joinThreads() = 0;
    virtual void detachThreads() = 0;
    virtual void runService() = 0;
    virtual void stopService() = 0;

};
}

#endif //WSSERVER_JOINABLE_H
