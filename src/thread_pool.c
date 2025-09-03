#include "thread_pool.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <SDL3/SDL_cpuinfo.h>
#include <libguile.h>

#include <stdio.h>

// TODO eventually this can be dynamically sized according to
// performance characteristics of the program. Force time sharing
// by increasing thread count if needed, even possibly disable
// thread pool if under utilized and resume CPU programs on the main
// thread for less overhead.

void destroy_socket_pairs(int (*pairs)[2], size_t count) {
  for (size_t i = 0; i < count; i++) {
      // TODO close can return with errors related to last write
      close(pairs[i][0]);
      close(pairs[i][1]);
  }
}

// Notify a waiting thread by sending it kill_msg via the socket, and then wait
// for the thread to respond to that by exiting, using SDL_Wait. We must
// therefore keep the sockets open before killing the threads.  Once the threads
// are killed, then we can call close() on both file descriptors.
void destroy_threads(ThreadPool* pool, size_t count) {
    for (size_t i = 0; i < count; i++) {
        // TODO could detach thread at this point if we don't want to wait, for instance
        // if we want to force quit.
        SCM kill_msg = SCM_UNDEFINED;
        {
            // TODO code duplication, make a SendSCMBlocking util func
            ssize_t bytes_written = 0;
            while (bytes_written < (ssize_t)sizeof(SCM)) {
                ssize_t write_size = write(pool->socket_pairs[i][0], (char*)&kill_msg, sizeof(SCM));
                if (write_size > 0) {
                    bytes_written += write_size;
                }
            }
        }
        SDL_WaitThread(pool->threads[i], NULL);
    }
}

void* inner_thread_func(void* data) {

    int socket = *(int*)data;

    while (true) {
        SCM msg = SCM_UNDEFINED;

        ssize_t bytes_read = 0;

        // TODO scm_without_guile will improve GC perf
        // TODO ensure that the socket is blocking.
        while (bytes_read != sizeof(SCM)) {
            
            ssize_t bytes_remaining = sizeof(SCM) - bytes_read;
            ssize_t read_size = read(socket, ((char*)&msg) + bytes_read, bytes_remaining);
            printf("Work Message Received\n");

            if (read_size > 0) {
                bytes_read += read_size;
            }

            // TODO not sure if errors are possible on this type of socket, and
            // it doesn't make much sense to handle them. We could report a failure
            // to the main thread which could attempt to restart the work or pass
            // the error on (and recover a thread in the thread pool).
        }

        if (SCM_UNDEFINED == msg) {
            printf("Shutdown worker\n");
            // Time to end this thread.
            break;
        }

        // TODO: is this msg object actually safe? What if we send the ack,
        // it gets unmarked on the parent thread, and we don't get back to
        // this thread before a garbage collection happens. This thread must
        // have the msg SCM at some level on the stack, but how does the GC
        // know if we haven't entered into a guile function yet?
        
        // Acknowledge the work has begun: (And that the sending thread can
        // remove protection from garbage collection):
        printf("Acknowledge work\n");
        SCM ack = SCM_UNDEFINED;
        {
            ssize_t bytes_written = 0;
            while (bytes_written < (ssize_t)sizeof(SCM)) {
                ssize_t write_size = write(socket, (char*)&ack, sizeof(SCM));
                if (write_size > 0) {
                    bytes_written += write_size;
                }
            }
        }


        // Time to do this thunk, and report the results.
        SCM result = scm_call_0(msg);
        {
            // TODO code duplication, make a SendSCMBlocking util func
            ssize_t bytes_written = 0;
            while (bytes_written < (ssize_t)sizeof(SCM)) {
                ssize_t write_size = write(socket, (char*)&result, sizeof(SCM));
                if (write_size > 0) {
                    bytes_written += write_size;
                }
            }
        }
    }

    return NULL;
}

int thread_func(void* data) {
    scm_with_guile(inner_thread_func, data);
    return 0;
}


bool init_thread_pool(ThreadPool* pool) {
    size_t thread_count = SDL_GetNumLogicalCPUCores();

    // TODO remove
    thread_count = 1;

    pool->threads = NULL;
    pool->thread_count = thread_count;
    pool->socket_pairs = NULL;


    pool->threads = malloc(sizeof(SDL_Thread*) * thread_count);
    if (pool->threads == NULL) {
        return false;
    }

    pool->socket_pairs = malloc(sizeof(SocketPair) * thread_count);
    if (pool->socket_pairs == NULL) {
        free(pool->threads);
        return false;
    }

    for (size_t i = 0; i < thread_count; i++) {
        // Create socket
        const int default_protocol = 0;
        // TODO figure out how to use SEQPACKET properly to house
        // a SCM value that is garbage collected.
        if (-1 == socketpair(AF_LOCAL, SOCK_STREAM, default_protocol, pool->socket_pairs[i])) {
            // TODO investigate errno
            destroy_threads(pool, i);
            destroy_socket_pairs(pool->socket_pairs, i);
            free(pool->threads);
            free(pool->socket_pairs);
            return false;
        }

        SDL_Thread* thread = SDL_CreateThread(thread_func, "worker", &pool->socket_pairs[i][1]);
        if (thread == NULL) {
            // TODO sdl err 
            destroy_threads(pool, i);
            destroy_socket_pairs(pool->socket_pairs, i + 1);
            free(pool->threads);
            free(pool->socket_pairs);
            return false;
        }

        pool->threads[i] = thread;
    }

    return true;
}

// Assumes that
void destroy_thread_pool(ThreadPool *pool) {

    // TODO close sockets, destroy threads.
    // TODO we can either wait on threads to complete, or detach
    // and force quit.
    destroy_threads(pool, pool->thread_count);
    destroy_socket_pairs(pool->socket_pairs, pool->thread_count);
    free(pool->socket_pairs);
    free(pool->threads);
}

