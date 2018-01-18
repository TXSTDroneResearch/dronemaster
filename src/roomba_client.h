#ifndef _ROOMBA_CLIENT_H_
#define _ROOMBA_CLIENT_H_

#include <cstddef>
#include <cstdint>

#include <netinet/in.h>
#include <semaphore.h>

// (simple) Roomba client. Represents a stream to a single roomba robot.
// This can send raw bytecode commands into the roomba robot. This is /very/
// insecure and is only used for development purposes.
class RoombaClient {
 public:
  RoombaClient(int socket);

  void Close();
  bool Send(const void* data, size_t len);

 private:
  int socket_ = 0;
  sockaddr_in client_addr_;
};

#endif  // _ROOMBA_CLIENT_H_