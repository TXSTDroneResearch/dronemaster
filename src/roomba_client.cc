#include "roomba_client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

RoombaClient::RoombaClient(int socket) : socket_(socket) {}

void RoombaClient::Close() {
    close(socket_);
}

bool RoombaClient::Send(const void* data, size_t len) {
    int ret = write(socket_, data, len);
    return ret >= 0;
}