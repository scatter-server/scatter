/*!
 * scatter
 * unid.cpp
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
    pid((uint16_t) (getpid() & 0x0000FFFF)),
    m_counter(1) {
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
    m_uuidBytes.store(
        ((randomBytes[0] << 24) | (randomBytes[1] << 16) | (randomBytes[2] << 8) | randomBytes[3]) & 0xFFFFFFFF,
        std::memory_order_relaxed
    );
}
wss::unid::id wss::unid::next() {

    uint32_t cnt = m_counter.load(std::memory_order_acquire);// force acquire latest inc

    // every N counter we need new uuid, just in case
    if (cnt % m_maxCounterUUID == 0 || cnt == m_maxCounter) {
        generateUUID();
    }

    // prevent overflow
    if (cnt == m_maxCounter) {
        cnt = 0;
    }

    cnt++;

    m_counter.store(cnt, std::memory_order_relaxed); // no matter, what thread will increment last value, relax sync
    return {
        (uint32_t) time(nullptr),
        m_uuidBytes.load(std::memory_order_acquire),
        pid,
        cnt
    };
}

wss::unid::id wss::unid::operator()() noexcept {
    return next();
}
