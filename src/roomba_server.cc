#include "roomba_server.h"

#include "logging.h"

#include <algorithm>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int SetBlocking(int socket, int blocking) {
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    printf("fcntl failed, errno = %s\n", strerror(errno));
    return flags;
  }

  if (blocking) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }

  int status = fcntl(socket, F_SETFL, flags);
  if (status == -1) {
    printf("fcntl failed, errno = %s\n", strerror(errno));
    return status;
  }

  return 0;
}

bool RoombaServer::Initialize(uint16_t port) {
  listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket_ < 0) {
    printf("Failed to create listen socket!\n");
    return false;
  }

  // Allow address reuse for debugging (non-fatal if it fails)
  int opt = 1;
  setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  SetBlocking(listen_socket_, 0);

  sockaddr_in server_addr;
  std::memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  // Bind the listen socket.
  int status =
      bind(listen_socket_, (const sockaddr *)&server_addr, sizeof(server_addr));
  if (status < 0) {
    printf("Failed to bind socket!\n");
    return false;
  }

  // Allow the socket to listen for requests (with a backlog of 5)
  if (listen(listen_socket_, 5) < 0) {
    printf("Failed to open the socket for listening!\n");
    return false;
  }

  // Setup epoll.
  efd_ = epoll_create1(0);

  // Add the listen socket to epoll's list.
  epoll_event evt;
  evt.data.fd = listen_socket_;
  evt.events = EPOLLIN | EPOLLET;  // Input, edge-triggered
  status = epoll_ctl(efd_, EPOLL_CTL_ADD, listen_socket_, &evt);
  if (status == -1) {
    PERROR("Failed to add listen socket to epoll list. errno = %s\n",
           strerror(errno));
    return false;
  }

  status = pipe(termination_pipe_);
  if (status != 0) {
    PERROR("Failed to create termination pipe. errno = %s\n", strerror(errno));
    return false;
  }

  evt.data.fd = termination_pipe_[0];
  evt.events = EPOLLIN | EPOLLET;
  status = epoll_ctl(efd_, EPOLL_CTL_ADD, termination_pipe_[0], &evt);
  if (status == -1) {
    PERROR("Failed to add termination pipe to epoll list. errno = %s\n",
           strerror(errno));
    return false;
  }

  // Handle up to 5 events at a time.
  events_.resize(5);

  // Okay. Create a thread to actually handle connections.
  worker_thread_ = std::thread(&RoombaServer::WorkerThreadFn, this);
  return true;
}

void RoombaServer::Shutdown() {
  write(termination_pipe_[1], "bye", 4);

  worker_thread_.join();
  if (efd_ != -1) {
    close(efd_);
    efd_ = -1;
  }

  if (listen_socket_ != -1) {
    close(listen_socket_);
    listen_socket_ = -1;
  }

  close(termination_pipe_[0]);
  close(termination_pipe_[1]);

  for (auto client : clients_) {
    client->Close();
    delete client;
  }
  clients_.clear();
}

void RoombaServer::Broadcast(void *data, size_t len) {
  std::lock_guard<std::mutex> lock(client_mutex_);
  for (auto client : clients_) {
    client->Send(data, len);
  }
}

size_t RoombaServer::GetNumClients() {
  std::lock_guard<std::mutex> lock(client_mutex_);
  return clients_.size();
}

void RoombaServer::WorkerThreadFn() {
  epoll_event evt;
  int status;

  while (true) {
    int n = epoll_wait(efd_, events_.data(), events_.size(), -1);
    for (int i = 0; i < n; i++) {
      if ((events_[i].events & EPOLLERR) || (events_[i].events & EPOLLHUP) ||
          !(events_[i].events & EPOLLIN)) {
        // Error on this socket.
        // TODO: Close the socket and terminate the client.
        printf("Error on socket described by %p\n", events_[i].data.ptr);

        if (events_[i].data.ptr != nullptr) {
          auto *client = reinterpret_cast<RoombaClient *>(events_[i].data.ptr);
          client->Close();

          // Erase the client from our vector.
          clients_.erase(std::remove(clients_.begin(), clients_.end(), client),
                         clients_.end());
          delete client;
        }
      } else if (events_[i].data.fd == listen_socket_) {
        // New client(s) connected. Loop and connect until we run out of new
        // clients.
        while (1) {
          sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);

          int sock =
              accept(listen_socket_, (sockaddr *)&client_addr, &client_len);
          if (sock < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
              // We've finished processing incoming connections.
              break;
            } else {
              printf("Error on accept! errno = %s\n", strerror(errno));
              break;
            }
          }

          SetBlocking(sock, 0);

          char hbuf[NI_MAXHOST], sbuf[NI_MAXHOST];
          status = getnameinfo((const sockaddr *)&client_addr, client_len, hbuf,
                               sizeof(hbuf), sbuf, sizeof(sbuf),
                               NI_NUMERICHOST | NI_NUMERICSERV);
          if (status == 0) {
            printf("New connection accepted from %s:%s\n", hbuf, sbuf);
          } else {
            printf("New connection accepted!\n");
          }

          auto *client = new RoombaClient(sock);
          evt.data.ptr = client;
          evt.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
          status = epoll_ctl(efd_, EPOLL_CTL_ADD, sock, &evt);
          if (status == -1) {
            printf("epoll_ctl failed, errno = %s\n", strerror(errno));
            delete client;
            continue;
          }

          // Drive forward
          /*
          const char data[] = "\x89\x01\xf4\x80\x00";
          client->Send(data, sizeof(data) - 1);
          */

          // Rotate (0x01F4 full speed, 0x0000 rotate -max)
          const char data[] = "\x89\x01\xf4\x00\x00";
          client->Send(data, sizeof(data) - 1);

          std::lock_guard<std::mutex> lock(client_mutex_);
          // Register the client internally.
          clients_.push_back(client);
        }
      } else if (events_[i].events & EPOLLRDHUP) {
        // Remote hangup.
        printf("Remote connection described by %p closed.\n",
               events_[i].data.ptr);

        if (events_[i].data.ptr != nullptr) {
          auto *client = reinterpret_cast<RoombaClient *>(events_[i].data.ptr);
          client->Close();

          // Erase the client from our vector.
          clients_.erase(std::remove(clients_.begin(), clients_.end(), client),
                         clients_.end());
          delete client;
        }
      } else if (events_[i].data.fd == termination_pipe_[0]) {
        // Termination signalled.
        return;
      } else {
        // We've handled all other events, this one means data is waiting.
        printf("Data waiting on socket described by %p\n", events_[i].data.ptr);
      }
    }
  }
}