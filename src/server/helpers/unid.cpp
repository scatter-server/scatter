/*!
 * wsserver
 * MessageID.cpp
 *
 * \date 2017
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */
#include <iostream>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <fmt/format.h>
#include "unid.h"

wss::unid::unid() :
    pid(getpid()),
    counter(1) {
    generateUUID();
}
void wss::unid::generateUUID() {
    boost::uuids::uuid id = boost::uuids::random_generator()();

    boost::random::random_device randDevice;
    boost::random::uniform_int_distribution<> indexDist(0, id.size());
    const int
        b1 = indexDist(randDevice),
        b2 = indexDist(randDevice),
        b3 = indexDist(randDevice),
        b4 = indexDist(randDevice);

    uint8_t randomBytes[4]{0, 0, 0, 0};
    randomBytes[0] = *(id.begin() + b1);
    randomBytes[1] = *(id.begin() + b2);
    randomBytes[2] = *(id.begin() + b3);
    randomBytes[3] = *(id.begin() + b4);

    /// upd.: tested 100 millions generations - no one collission
    /// is this a good idea? i'm lazy to read whole rfc
    /// \todo use unique machine id (mac address, cpu serial or somthing else) instead of uuid bytes
    /// \link https://tools.ietf.org/html/rfc4122
    uuidBytes = (randomBytes[0] << 24) | (randomBytes[1] << 16) | (randomBytes[2] << 8) | randomBytes[3];
}
wss::unid::id wss::unid::next() {
    // every 1000 ids generating new uuid
    if (counter.load() % 1000 == 0) {
        generateUUID();
    }

    // prevent overflow
    if (counter + 1 == 0xFFFFFFFF) {
        counter = 0;
    }

    counter++;

    //@formatter:off
    uint32_t part1 = (int)time(nullptr)      & 0xFFFFFFFF;  // 4 bytes
    uint32_t part2 = uuidBytes               & 0xFFFFFFFF;  // 4 bytes
    uint16_t part3 = (uint16_t)         (pid & 0x0000FFFF); // 2 bytes
    uint32_t part4 = counter;                               // 4 bytes
    //@formatter:on

    return {part1, part2, part3, part4};
}

wss::unid::id wss::unid::operator()() noexcept {
    return next();
}
