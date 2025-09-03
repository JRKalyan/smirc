#ifndef SMIRC_THREAD_POOL_H
#define SMIRC_THREAD_POOL_H
#include <SDL3/SDL_thread.h>

typedef int SocketPair[2];

// TODO, investigate shared memory if message
// passing to threads via sockets proves slow.
// Sockets were preferred initially because
// I can easily use epoll and there should be
// code re-use for over-network IPC

typedef struct ThreadPool {
    size_t thread_count;
    SDL_Thread** threads;
    SocketPair* socket_pairs;
    //int (*socket_pairs)[2];
} ThreadPool;


bool init_thread_pool(ThreadPool* pool);

void destroy_thread_pool(ThreadPool* pool);

#endif
