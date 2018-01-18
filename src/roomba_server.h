#ifndef _ROOMBA_SERVER_H_
#define _ROOMBA_SERVER_H_

#include <sys/epoll.h>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "roomba_client.h"

// Roomba server. This handles connections with Roombas, as well as sending
// commands to specific Roombas.
class RoombaServer {
 public:
  bool Initialize(uint16_t port);
  void Shutdown();

  void Broadcast(void* data, size_t len);

  // Gets the number of clients at the time of this call.
  // WARNING: The actual amount can change at any point!
  size_t GetNumClients();

 private:
  void WorkerThreadFn();

  int efd_ = -1;
  int listen_socket_ = -1;
  int termination_pipe_[2];

  std::vector<epoll_event> events_;
  std::vector<RoombaClient*> clients_;
  std::mutex client_mutex_;
  std::thread worker_thread_;
};

#endif  // _ROOMBA_SERVER_H_