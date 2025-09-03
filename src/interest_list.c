#include "interest_list.h"

#include <stddef.h>

bool init_interest_list(InterestList* list) {
    list->fd_count = 0;
    list->epoll_event_count = 0;

    // argument is ignored but must be greater than 0
    list->epoll_fd = epoll_create(1);

    return true;
}

bool add_interest(InterestList* list, int fd) {
    if (list->fd_count >= INTEREST_LIST_MAX) {
        return false;
    }

    for (uint32_t i = 0; i < list->fd_count; i++) {
        if (list->fds[i] == fd) {
            list->refs[i]++;
            return true;
        }
    }

    struct epoll_event event  = {
        .events = EPOLLIN,
        .data = { .fd = fd }
    };
    epoll_ctl(list->epoll_fd, EPOLL_CTL_ADD, fd, &event);
    list->fds[list->fd_count] = fd;
    list->refs[list->fd_count] = 1;
    list->fd_count++;

    return true;
}

void remove_interest(InterestList* list, int fd) {
    for (uint32_t i = 0; i < list->fd_count; i++) {
        if (list->fds[i] == fd) {
            if (--list->refs[i] == 0) {
                epoll_ctl(list->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                list->fd_count--;
                list->refs[i] = list->refs[list->fd_count];
                list->fds[i] = list->fds[list->fd_count];
            }
        }
    }
}

void wait_interest_list(InterestList* list, int timeout_ms) {
    list->epoll_event_count = epoll_wait(
        list->epoll_fd,
        list->epoll_events,
        EPOLL_MAX_EVENTS,
        timeout_ms);

}
