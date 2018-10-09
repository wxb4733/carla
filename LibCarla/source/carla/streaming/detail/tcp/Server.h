// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/NonCopyable.h"
#include "carla/Time.h"
#include "carla/streaming/detail/tcp/ServerSession.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace carla {
namespace streaming {
namespace detail {
namespace tcp {

  class Server : private NonCopyable {
  public:

    using endpoint = boost::asio::ip::tcp::endpoint;
    using protocol_type = endpoint::protocol_type;

    explicit Server(boost::asio::io_service &io_service, endpoint ep);

    ~Server();

    /// Set session time-out. Applies only to newly created sessions. By default
    /// the time-out is set to 10 seconds.
    void SetTimeout(time_duration timeout) {
      _timeout = timeout;
    }

    /// Start listening for connections, on each new connection @a callback is
    /// called.
    template <typename Functor>
    void Listen(Functor callback) {
      _acceptor.get_io_service().post([=]() { OpenSession(_timeout, callback); });
    }

  private:

    void OpenSession(time_duration timeout, ServerSession::callback_function_type callback);

    bool RegisterSession(std::shared_ptr<ServerSession> session);

    boost::asio::ip::tcp::acceptor _acceptor;

    std::atomic<time_duration> _timeout;

    // The mutex only protects the list of active sessions.
    std::mutex _mutex;

    bool _done = false;

    std::unordered_map<ServerSession *, std::weak_ptr<ServerSession>> _active_sessions;
  };

} // namespace tcp
} // namespace detail
} // namespace streaming
} // namespace carla
