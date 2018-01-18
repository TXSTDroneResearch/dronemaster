#include "service_broadcast_avahi.h"

#include <cstring>

#include "logging.h"

#ifdef RC_USE_AVAHI

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/thread-watch.h>


struct InternalAvahiService {
  AvahiServiceBroadcaster* sb;
  AvahiEntryGroup* group;
  ServiceDesc svc;
};

void AvahiServiceBroadcaster::EntryGroupCallback(AvahiEntryGroup* g,
                                            AvahiEntryGroupState state,
                                            void* userdata) {
  InternalAvahiService* data = (InternalAvahiService*)userdata;

  // ...
  switch (state) {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
      /* The entry group has been established successfully */
      break;
    case AVAHI_ENTRY_GROUP_COLLISION: {
      char* n;
      /* A service name collision with a remote service
       * happened. Let's pick a new name */
      n = avahi_alternative_service_name(data->svc.name);
      avahi_free((void*)data->svc.name);
      data->svc.name = n;
      PINFO("Service name collision, renaming service to '%s'\n",
            data->svc.name);

      /* And recreate the services */
      int ret = 0;
      if (ret == 0) {
        ret = avahi_entry_group_add_service(
            g, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
            data->svc.name, data->svc.type, data->svc.domain, data->svc.host,
            data->svc.port);
        if (ret < 0) {
          PERROR("Failed to add service %s to an entry group! (avahi: %s)\n",
                 data->svc.name, avahi_strerror(ret));
        }
      }

      if (ret == 0) {
        ret = avahi_entry_group_commit(g);
        if (ret < 0) {
          PERROR("Failed to commit entry group! (avahi: %s)\n",
                 avahi_strerror(ret));
        }
      }
      break;
    }
    case AVAHI_ENTRY_GROUP_FAILURE:
      fprintf(
          stderr, "Entry group failure: %s\n",
          avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
      break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:
      break;
  }
}

void AvahiServiceBroadcaster::ClientCallback(AvahiClient* c, AvahiClientState state,
                                        void* userdata) {
  AvahiServiceBroadcaster* sb = (AvahiServiceBroadcaster*)userdata;

  assert(c);
  /* Called whenever the client or server state changes */
  switch (state) {
    case AVAHI_CLIENT_S_RUNNING: {
      /* The server has startup successfully and registered its host
       * name on the network, so it's time to create our services */
      int ret = avahi_threaded_poll_start(sb->threaded_poll_);
    } break;
    case AVAHI_CLIENT_FAILURE:
      fprintf(stderr, "Client failure: %s\n",
              avahi_strerror(avahi_client_errno(c)));
      avahi_threaded_poll_quit(sb->threaded_poll_);
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
      // if (sb->group) avahi_entry_group_reset(sb->group);
      break;
    case AVAHI_CLIENT_CONNECTING:
      break;
  }
}

int AvahiServiceBroadcaster::Initialize() {
  threaded_poll_ = avahi_threaded_poll_new();
  if (!threaded_poll_) {
    return -1;
  }

  int ret = 0;
  avahi_client_ = avahi_client_new(
      avahi_threaded_poll_get(threaded_poll_), AvahiClientFlags(0),
      &AvahiServiceBroadcaster::ClientCallback, this, &ret);
  if (!avahi_client_) {
    PERROR("Failed to create avahi client! (avahi: %s)", avahi_strerror(ret));
    return -1;
  }
}

void AvahiServiceBroadcaster::Shutdown() {
  // TODO: Shutdown...
}

uintptr_t AvahiServiceBroadcaster::AddService(const ServiceDesc& service) {
  avahi_threaded_poll_lock(threaded_poll_);

  AvahiEntryGroup* group = avahi_entry_group_new(
      avahi_client_, &AvahiServiceBroadcaster::EntryGroupCallback, this);
  if (!group) {
    return 0;
  }

  InternalAvahiService* service_data = new InternalAvahiService;
  std::memset(service_data, 0, sizeof(InternalAvahiService));
  std::memcpy(&service_data->svc, &service, sizeof(ServiceDesc));
  service_data->sb = this;
  service_data->group = group;

  int ret = avahi_entry_group_add_service(
      group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0),
      service.name, service.type, service.domain, service.host, service.port);
  if (ret < 0) {
    PERROR("Failed to add service %s to an entry group! (avahi: %s)\n",
           service.name, avahi_strerror(ret));

    delete service_data;
    return 0;
  }

  if (service.name) {
    size_t len = std::strlen(service.name) + 1;

    service_data->svc.name = (const char*)avahi_malloc(len);
    std::memcpy((void*)service_data->svc.name, service.name, len);
  }

  if (service.type) {
    size_t len = std::strlen(service.type) + 1;

    service_data->svc.type = (const char*)avahi_malloc(len);
    std::memcpy((void*)service_data->svc.type, service.type, len);
  }

  if (service.domain) {
    size_t len = std::strlen(service.domain) + 1;

    service_data->svc.domain = (const char*)avahi_malloc(len);
    std::memcpy((void*)service_data->svc.domain, service.domain, len);
  }

  if (service.host) {
    size_t len = std::strlen(service.host) + 1;

    service_data->svc.host = (const char*)avahi_malloc(len);
    std::memcpy((void*)service_data->svc.host, service.host, len);
  }

  ret = avahi_entry_group_commit(group);
  if (ret < 0) {
    if (service_data->svc.name) {
      avahi_free((void*)service_data->svc.name);
    }
    if (service_data->svc.type) {
      avahi_free((void*)service_data->svc.type);
    }
    if (service_data->svc.domain) {
      avahi_free((void*)service_data->svc.domain);
    }
    if (service_data->svc.host) {
      avahi_free((void*)service_data->svc.host);
    }
    delete service_data;

    PERROR("Failed to commit entry group! (avahi: %s)\n", avahi_strerror(ret));
    return 0;
  }

  avahi_threaded_poll_unlock(threaded_poll_);
}

int AvahiServiceBroadcaster::RemoveService(uintptr_t id) {
  // TODO...
  return -1;
}

#endif