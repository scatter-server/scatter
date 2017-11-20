/**
 * wsserver_standalone
 * HttpClient.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_STANDALONE_HTTPCLIENT_H
#define WSSERVER_STANDALONE_HTTPCLIENT_H

#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <istream>
#include <ostream>
#include "QueryBuilder.hpp"

namespace wss {
namespace client {

using namespace boost::asio;
using namespace boost::asio::ip;

class Request {

};

struct Response {
  int status;
  std::string statusMessage;
  std::string data;
  std::vector<std::string> headers;
};

class HttpClient {
 private:
    io_service ioService;

 public:
    Response call(wss::client::QueryBuilder qb) {
        // Get a list of endpoints corresponding to the server name.
        ip::tcp::resolver resolver(ioService);
        ip::tcp::resolver::query query(qb.getHost(), qb.getProtocol());
        ip::tcp::resolver::iterator endpointIterator = resolver.resolve(query);

        // Try each endpoint until we successfully establish a connection.
        ip::tcp::socket socket(ioService);
        boost::asio::connect(socket, endpointIterator);

        boost::asio::streambuf request;
        std::ostream requestStream(&request);
        qb.buildOStream(&requestStream);

        // Send the request.
        boost::asio::write(socket, request);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream responseStream(&response);
        std::string httpVersion;
        responseStream >> httpVersion;
        int statusCode;
        responseStream >> statusCode;
        std::string statusMessage;
        std::getline(responseStream, statusMessage);

        if (!responseStream || httpVersion.substr(0, 5) != "HTTP/") {
            return {-1, "Invalid response", ""};
        }

        if (statusCode != 200) {
            return {statusCode, "HTTP error " + std::to_string(statusCode), ""};
        }

        Response resp = {statusCode, statusMessage, ""};
        std::stringstream dataStream;

        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        std::string header;
        while (std::getline(responseStream, header) && header != "\r") {
            resp.headers.push_back(header);
        }

        // Write whatever content we already have to output.
        if (response.size() > 0) {
            dataStream << &response;
        }

        // Read until EOF, writing data to output as we go.
        boost::system::error_code error;
        while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
//            std::cout << &response;
            dataStream << &response;
        }

        resp.data = dataStream.str();

        if (error != boost::asio::error::eof)
            throw boost::system::system_error(error);

        return resp;
    }
};

}
}

#endif //WSSERVER_STANDALONE_HTTPCLIENT_H
