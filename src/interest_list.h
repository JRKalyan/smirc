#ifndef SMIRC_INTEREST_LIST_H
#define SMIRC_INTEREST_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/epoll.h>

// TODO dynamically allocate
// TODO we may want to register atoms directly as a callback
// instead of scanning atom list for potential updates, but
// that may only pay off at very large number of fds
// (which is possible in some applications)
#define EPOLL_MAX_EVENTS 10
#define INTEREST_LIST_MAX 100
typedef struct {
    uint32_t fd_count;
    int epoll_fd;
    int fds[INTEREST_LIST_MAX];
    int refs[INTEREST_LIST_MAX];
    struct epoll_event epoll_events[EPOLL_MAX_EVENTS];
    int epoll_event_count;

    // TODO incorporate signal refcounting, so that the sigset for the signalfd fd
    // doesn't get incorrectly cleared while there is still someone wanting to handle
    // that signal.

    // That could be done in a different file if it fits better.
} InterestList;

bool init_interest_list(InterestList* list);
bool add_interest(InterestList* list, int fd);
void remove_interest(InterestList* list, int fd);
void wait_interest_list(InterestList* list, int timeout_ms);

#endif
