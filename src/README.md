# Drone Master Source

## roomba_server.cc

Houses the core server logic. This server spawns a thread that waits on
and handles all network events (i.e. connections/disconnections/receiving data)
using the linux kernel's epoll functionality (basically a really fancy select call)

Most code is fairly well commented, and should be pretty easy to follow.

## roomba_client.cc

This is a wrapper for Roomba clients.

## service_broadcast_*.cc

These implement `ServiceBroadcaster` for Avahi on both linux hosts and the Dragon (Bebop)
host.