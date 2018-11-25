/*!
 * scatter.
 * BaseSocketServer.h
 *
 * Virtual class to make single proxy intance of variable instances of socket servers (for example: ssl and without ssl support)
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef SCATTER_BASESERVER_H
#define SCATTER_BASESERVER_H

#include <memory>

namespace wss {
namespace server {

class BaseServer {
 public:
    /*!
     * \brief Start server
     */
    virtual void start() = 0;
    /*!
     * \brief Stop server
     */
    virtual void stop() = 0;

    /*!
     * \brief Accept connections
     */
    virtual void accept() = 0;
};


} //namespace server
} //namespace wss

#endif //SCATTER_BASESERVER_H
