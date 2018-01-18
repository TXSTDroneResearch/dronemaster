#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/thread-watch.h>

#include "roomba_server.h"

struct AvahiData {
  AvahiThreadedPoll* thread_poll;
  AvahiEntryGroup* group;

  char* name;
};

static void register_services(AvahiEntryGroup* g) {
  int ret;
  if ((ret = avahi_entry_group_add_service(
           g, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
           "Roomba Controller", "_roomba._tcp", nullptr, nullptr, 1444,
           nullptr)) < 0) {
    printf("Failed to add service. ret = %s\n", avahi_strerror(ret));
    return;
  }

  if ((ret = avahi_entry_group_commit(g)) < 0) {
    printf("Failed to commit entry group. ret = %s\n", avahi_strerror(ret));
    return;
  }

  printf("Service registered via Avahi.\n");
}

static void entry_group_callback(AvahiEntryGroup* g, AvahiEntryGroupState state,
                                 AVAHI_GCC_UNUSED void* userdata) {
  AvahiData* data = (AvahiData*)userdata;

  // ...
  switch (state) {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
      /* The entry group has been established successfully */
      break;
    case AVAHI_ENTRY_GROUP_COLLISION: {
      char* n;
      /* A service name collision with a remote service
       * happened. Let's pick a new name */
      n = avahi_alternative_service_name(data->name);
      avahi_free(data->name);
      data->name = n;

      
      fprintf(stderr, "Service name collision, renaming service to '%s'\n",
              data->name);
      /* And recreate the services */
      register_services(g);
      break;
    }
    case AVAHI_ENTRY_GROUP_FAILURE:
      fprintf(
          stderr, "Entry group failure: %s\n",
          avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
      /* Some kind of failure happened while we were registering our services */
      avahi_threaded_poll_quit(data->thread_poll);
      break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:
      break;
  }
}

static void client_callback(AvahiClient* c, AvahiClientState state,
                            AVAHI_GCC_UNUSED void* userdata) {
  AvahiData* data = (AvahiData*)userdata;

  assert(c);
  /* Called whenever the client or server state changes */
  switch (state) {
    case AVAHI_CLIENT_S_RUNNING:
      /* The server has startup successfully and registered its host
       * name on the network, so it's time to create our services */
      if (!data->group) {
        data->group = avahi_entry_group_new(c, entry_group_callback, data);
      }

      if (avahi_entry_group_is_empty(data->group)) {
        // Register our services...
        register_services(data->group);
      }

      break;
    case AVAHI_CLIENT_FAILURE:
      fprintf(stderr, "Client failure: %s\n",
              avahi_strerror(avahi_client_errno(c)));
      avahi_threaded_poll_quit(data->thread_poll);
      break;
    case AVAHI_CLIENT_S_COLLISION:
    /* Let's drop our registered services. When the server is back
     * in AVAHI_SERVER_RUNNING state we will register them
     * again with the new host name. */
    case AVAHI_CLIENT_S_REGISTERING:
      /* The server records are now being established. This
       * might be caused by a host name change. We need to wait
       * for our own records to register until the host name is
       * properly established. */
      if (data->group) avahi_entry_group_reset(data->group);
      break;
    case AVAHI_CLIENT_CONNECTING:
      break;
  }
}

int main(int argc, char* argv[]) {
  AvahiClient* client = nullptr;
  AvahiThreadedPoll* thread_poll = nullptr;
  int error;

  RoombaServer roomba_server;
  if (!roomba_server.Initialize(1444)) {
    printf("Failed to start the roomba server.\n");
    return 1;
  }

  // Allocate the simple polling loop
  thread_poll = avahi_threaded_poll_new();
  if (!thread_poll) {
    printf("Failed to allocate threaded poller.\n");
    return 1;
  }

  AvahiData data;
  std::memset(&data, 0, sizeof(data));
  data.thread_poll = thread_poll;

  client =
      avahi_client_new(avahi_threaded_poll_get(thread_poll),
                       AvahiClientFlags(0), client_callback, &data, &error);
  if (!client) {
    printf("Failed to create client. %s\n", avahi_strerror(error));
    avahi_threaded_poll_free(thread_poll);
    thread_poll = nullptr;
  }

  if (thread_poll != nullptr) {
    avahi_threaded_poll_start(thread_poll);
  }

  // TODO: Run input on this thread
  // ...
  printf("Listening for new connections.\n");
  printf("Press the any key to exit.\n");
  getchar();

  if (client) {
    avahi_threaded_poll_stop(thread_poll);
    avahi_client_free(client);
  }

  roomba_server.Shutdown();
  return 0;
}