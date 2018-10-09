// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "carla/streaming/detail/tcp/Server.h"

#include "carla/Logging.h"

#include <memory>

namespace carla {
namespace streaming {
namespace detail {
namespace tcp {

  Server::Server(boost::asio::io_service &io_service, endpoint ep)
    : _acceptor(io_service, std::move(ep)),
      _timeout(time_duration::seconds(10u)) {}

  Server::~Server() {
    std::lock_guard<std::mutex> lock(_mutex);
    _done = true;
    for (auto &pair : _active_sessions) {
      auto session = pair.second.lock();
      if (session != nullptr) {
        session->Close();
      }
    }
  }

  void Server::OpenSession(
      const time_duration timeout,
      ServerSession::callback_function_type callback) {
    using boost::system::error_code;

    auto session = std::make_shared<ServerSession>(_acceptor.get_io_service(), timeout);

    auto handle_query = [this, callback, session](const error_code &ec) {
      if (!ec) {
        if (RegisterSession(session)) {
          session->Open(callback);
        }
      } else {
        log_error("tcp accept error:", ec.message());
      }
    };

    _acceptor.async_accept(session->_socket, [=](error_code ec) {
      // Handle query and open a new session immediately.
      _acceptor.get_io_service().post([=]() { handle_query(ec); });
      OpenSession(timeout, callback);
    });
  }

  bool Server::RegisterSession(std::shared_ptr<ServerSession> session) {
    std::lock_guard<std::mutex> lock(_mutex);
    // Clean up dead sessions.
    for(auto it = _active_sessions.begin(); it != _active_sessions.end(); ) {
      auto session = it->second.lock();
      if(session == nullptr) {
        it = _active_sessions.erase(it);
      } else {
        ++it;
      }
    }
    // Add the new one.
    if (!_done) {
      _active_sessions.emplace(session.get(), session);
    }
    return !_done;
  }

} // namespace tcp
} // namespace detail
} // namespace streaming
} // namespace carla
