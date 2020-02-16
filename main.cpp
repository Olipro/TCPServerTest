#include <boost/asio.hpp>

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

using namespace boost::asio;
using tcp = boost::asio::ip::tcp;

struct IO {
    static thread_local inline io_context io;
    static thread_local inline tcp::acceptor acceptor{io};
    static inline std::vector<std::reference_wrapper<tcp::acceptor>> acceptors;
    static inline std::mutex lock;
    static inline std::atomic_int counter;
};

struct SessionContext {
    tcp::socket sock;
    tcp::endpoint client;
    char buf[3];
    explicit SessionContext(io_context& io) : sock{io} {}
};

void EchoLoop(std::unique_ptr<SessionContext>&& ctxPtr) {
    auto& sock = ctxPtr->sock;
    auto& buf  = ctxPtr->buf;
    sock.async_read_some(buffer(buf, 512), [ctxPtr = std::move(ctxPtr)] (auto& ec, auto bytes) mutable {
        if (ec)
            return;
        IO::counter++;
        auto& sock = ctxPtr->sock;
        auto& buf = ctxPtr->buf;
        async_write(sock, buffer(buf, bytes), [ctxPtr = std::move(ctxPtr)] (auto& ec, auto bytes) mutable {
            if (ec)
                return;
            EchoLoop(std::move(ctxPtr));
        });
    });
}

void AcceptConnection(io_context& io, tcp::acceptor& acceptor) {
    auto ctxPtr = std::make_unique<SessionContext>(io);
    auto& sock = ctxPtr->sock;
    auto& ep = ctxPtr->client;
    acceptor.async_accept(sock, ep, [ctxPtr = std::move(ctxPtr), &io, &acceptor] (const auto& ec) mutable {
        if (ec)
            return;
        AcceptConnection(io, acceptor);
        EchoLoop(std::move(ctxPtr));
    });
}

using reuse_port = boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT>;

int main() {
    std::vector<std::thread> threads;
    for (auto i = 0; i < 8; ++i)
        threads.emplace_back([] {
            io_context& io = IO::io;
            tcp::acceptor& acceptor = IO::acceptor;
            acceptor.open(tcp::v6());
            acceptor.set_option(ip::v6_only{false});
            acceptor.set_option(ip::tcp::socket::reuse_address{true});
            acceptor.set_option(reuse_port{true});
            acceptor.bind(tcp::endpoint{tcp::v6(), 12345});
            acceptor.listen(100);
            AcceptConnection(io, acceptor);
            {
                std::unique_lock l{IO::lock};
                IO::acceptors.emplace_back(std::ref(acceptor));
            }
            io.run();
        });
    std::signal(SIGINT, [] (auto signal) {
        if (signal == SIGINT) {
            std::unique_lock l{IO::lock};
            for (auto& acceptor : IO::acceptors)
                acceptor.get().cancel();
        }
    });
    for (auto& thread : threads)
        thread.join();
    std::cout << "count: " << IO::counter << '\n';
    return 0;
}
