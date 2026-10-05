#include "unix_connection.h"

#include <unistd.h>
#include <cerrno>
#include <poll.h>
#include <simpleipc/common/io_handler.h>

using namespace simpleipc;

void unix_connection::register_io_handler() {
    io_handler::get_instance().add_socket(fd, [this](int) { handle_data_available(); }, [this](int) { on_close(); });
}

void unix_connection::unregister_io_handler() {
    io_handler::get_instance().remove_socket(fd);
}

void unix_connection::send_data(const char* data, size_t datalen) {
    size_t sent = 0;
    while (sent < datalen) {
        ssize_t count = write(fd, data + sent, datalen - sent);
        if (count > 0) {
            sent += static_cast<size_t>(count);
            continue;
        }
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd ready{fd, POLLOUT, 0};
            int result;
            do { result = poll(&ready, 1, 3000); } while (result < 0 && errno == EINTR);
            if (result > 0 && (ready.revents & POLLOUT)) continue;
        }
        throw std::runtime_error("write() failed before the complete IPC message was sent");
    }
}

ssize_t unix_connection::read_data(char* data, size_t datalen) {
    return read(fd, data, datalen);
}
