#ifndef TCP_CLIENT
#define TCP_CLIENT

#include <boost/asio.hpp>
#include <boost/crc.hpp>

namespace Achal {

namespace TCP {

class Node {
 public:
  std::string ip;
  std::string port;

  operator std::string() const {
    return ip + "::" + port;
  }

  std::string get_uri() {
    return "tcp://" + ip + ":" + port;
  }

  std::string get_url() {
    return "http://" + ip + ":" + port;
  }
};

}  // namespace TCP

}  // namespace Achal

#endif
