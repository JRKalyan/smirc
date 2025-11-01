#include <bits/types/siginfo_t.h>
#include <bits/types/sigset_t.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/signalfd.h>
#include <libguile.h>

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "context.h"
#include "stf.h"
#include "shader.h"
#include "interest_list.h"
#include "thread_pool.h"


// TIME SHENANIGANS
// ------------------------------------------------------------

// TODO consider using only milliseconds, since epoll uses it

#define MSEC_PER_SEC 1000
#define NSEC_PER_SEC 1000000000
#define NSEC_PER_MSEC 1000000
void subtract_time(struct timespec* t1, const struct timespec* t2) {
    t1->tv_sec -= t2->tv_sec;
    t1->tv_nsec -= t2->tv_nsec;

    if (t1->tv_nsec < 0) {
        t1->tv_nsec += NSEC_PER_SEC;
        t1->tv_sec -= 1;
    }

    if (t1->tv_nsec >= NSEC_PER_SEC) {
        int nsec = t1->tv_nsec;
        t1->tv_nsec = nsec % NSEC_PER_SEC;
        t1->tv_sec = t1->tv_sec + (nsec / NSEC_PER_SEC);
    }
}

int timespec_to_ms(struct timespec* time) {
    return time->tv_sec * MSEC_PER_SEC + (time->tv_nsec / NSEC_PER_MSEC);
}

void timespec_from_ms(struct timespec* time, int ms) {
    time->tv_sec = ms / MSEC_PER_SEC;
    time->tv_nsec = (ms % MSEC_PER_SEC) * NSEC_PER_MSEC;
}
// ------------------------------------------------------------


// TODO remove default signal handlers for sigint etc:
// sigint the platform should handle, but user code could
// also be notified if there is an atom waiting.

typedef struct UpdateInfo {
    const InterestList* interest_list;
    const struct timespec* elapsed;
} UpdateInfo;

typedef struct Atom {
    struct timespec timer; // Time until this atom should be updated.
    SCM value; // Object returned to SCM TODO possibly C retval with void*
    SCM ro_next; // function we are bound to, optionally not bound to anything (todo nullable scm?)
    SCM ro_parameters; // Read only optional parameters
    bool is_complete; // atom has been completed
    void (*update) (struct Atom* atom, const UpdateInfo* update_info);
    int fd;
    size_t bytes_read;
    // TODO possibly C retval, SCM update func (so that guile can register polling or signal atoms),
    // TODO possible shortcut an Atom to register once this one completes, or a C func to call
    // instead of SCM next.
} Atom;


// TODO land on a design for atoms which have their own data needs for C:
// void ptr with malloced data? cram all fields into Atom and let them choose
// what they need? (what I'm doing now) union of different structs which give
// different atoms their needed C data, effectivel cramming but using only the
// space for the max sized atom type?


// NOTE: if we want a non-pure guile layer to be able to register it's own scm
// functions as an update(), then we also need to expose the Atom type, since
// the update works on that type.

// TODO foreign function? Does that get me anything?
typedef struct AtomDesc {
    void (*init)(struct Atom*, SCM);
} AtomDesc;

static SCM atom_desc_type;

void init_atom_desc_type() {
    SCM name = scm_from_utf8_symbol("atom_desc");
    SCM slots = scm_list_1(scm_from_utf8_symbol("data"));
    scm_t_struct_finalize finalizer = NULL;
    atom_desc_type = scm_make_foreign_object_type(name, slots, finalizer);
}

// TODO error on alloc failure
SCM make_atom_desc() {
    // TODO can we avoid malloc for function pointer? stuff into scm value or no? as int?
    AtomDesc* atom_desc = scm_gc_malloc(sizeof(AtomDesc), "atom_desc"); // TODO error check?
    return scm_make_foreign_object_1(atom_desc_type, atom_desc);
}



// TODO atom scope definitions?
// Possible have each atom have a clearly defined
// dependency graph and mutation graph (the things that
// it mutates)
// Then we can reason restrict programs to not mutate the
// dependency while something else is using it.

void update_atoms(
    Atom* atoms,
    uint32_t count,
    const UpdateInfo* update_info) {

    for (uint32_t i = 0; i < count; i++) {
        Atom* atom = &atoms[i];
        // TODO let the atoms decide to subtract time.
        atom->update(atom, update_info);
    }
}


// STATIC DATA
// -----------------------------
// TODO find a proper home for these data,
// which may be better served as heap allocated
// or stack allocated, to avoid bogging down the
// guile garbage collector. OR find a way to tell
// guile not to look in static data and when necessary
// prefer manually marking SCM values which should be
// pretty straight forward.

#define TEMP_ATOM_MAX 50 // TODO dynamically allocate.
static Atom s_atoms[TEMP_ATOM_MAX];
static uint32_t s_atom_count = 0;

static InterestList s_interest_list;

static ThreadPool s_thread_pool;

#define SIGNALFD_MAX_SIGINFO 4
static int s_signal_fd;
static sigset_t s_sig_set;
// -----------------------------


// ATOM UTIL
// ------------------------------------------------

#define UPD_SUBELAPSED() \
    subtract_time(&atom->timer, update_info->elapsed);


// ------------------------------------------------


// SLEEP ATOM
// ------------------------------------------------
void sleep_update(struct Atom* atom, const UpdateInfo* update_info) {
    UPD_SUBELAPSED();
    if (atom->timer.tv_sec < 0) {
        atom->is_complete = true;
        atom->value = scm_from_int(0); // TODO properly find out how to return nothing to scm
    }
}

void sleep_init(struct Atom* atom, SCM parameters) {
    int ms = scm_to_int(parameters); // TODO error check?
    timespec_from_ms(&atom->timer, ms);
    atom->is_complete = false;
    atom->update = sleep_update;
}

SCM sleep_atom_desc() {
    SCM scm_atom_desc = make_atom_desc();
    AtomDesc* atom_desc = scm_foreign_object_ref(scm_atom_desc, 0);
    atom_desc->init = sleep_init;
    return scm_atom_desc;
}
// ------------------------------------------------

// READ ATOM
// ------------------------------------------------

void read_update(struct Atom* atom, const UpdateInfo* update_info) {

    const struct epoll_event* fd_event = NULL;
    for (int i = 0; i < update_info->interest_list->epoll_event_count; i++) {
        const struct epoll_event* event = &update_info->interest_list->epoll_events[i];
        if (event->data.fd == atom->fd) {
            fd_event = event;
        }
    }

    if (fd_event == NULL) {
        return;
    }

    if ((fd_event->events & EPOLLIN) == 0) {
        return;
    }

    signed char* buffer = SCM_BYTEVECTOR_CONTENTS(atom->value);
    size_t bytes_remaining = SCM_BYTEVECTOR_LENGTH(atom->value) - atom->bytes_read;
    ssize_t read_size = read(fd_event->data.fd, buffer + atom->bytes_read, bytes_remaining);
    if (read_size == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Try again later
            return;
        }
        else {
            // Something worse has happened, our read as finished. TODO
            // communicate to SCM something is wrong properly, or panic
            // the program and give error information.
            atom->is_complete = true;
            remove_interest(&s_interest_list, STDIN_FILENO);
            return;
        }
    }

    atom->bytes_read += read_size;

    // TODO probably indicate final read size to scheme in case EOF when
    // reading from file.
    if (read_size == 0 || (size_t)read_size == bytes_remaining) {
        atom->is_complete = true;
        remove_interest(&s_interest_list, STDIN_FILENO);
    }
}

void read_init(struct Atom* atom, SCM parameters) {
    size_t byte_count = scm_to_size_t(parameters);
    atom->value = scm_c_make_bytevector(byte_count); // TODO error check malloc

    atom->timer.tv_sec = -1; // Necessary to tell update loop we do not need to wait.
    atom->is_complete = false;
    atom->update = read_update;
    atom->bytes_read = 0;
    atom->fd = STDIN_FILENO;

    // TODO allow atoms to specify event_data possibly
    add_interest(&s_interest_list, STDIN_FILENO);
}

SCM read_atom_desc() {
    SCM scm_atom_desc = make_atom_desc();
    AtomDesc* atom_desc = scm_foreign_object_ref(scm_atom_desc, 0);
    atom_desc->init = read_init;
    return scm_atom_desc;
}

// ------------------------------------------------


// SEND ATOM
// ------------------------------------------------
// ------------------------------------------------


// RECEIVE ATOM
// ------------------------------------------------
// ------------------------------------------------


// TODO schedule atom multithreaded support:
// if you're a child thread, then schedule_atom
// called by the guile world program processor should
// actually be using a socket to communicate to the main
// thread about the atom to schedule. Since we communicate
// via SCM objects, we can just make a thunk to execute
// on the main thread instead, and pass it via the socket
// (ensuring it survives garbage collection) and then
// execute it on the main thread.
// In fact process-world-program can entirely be on the main
// thread, but we may as well do that on a child thread.

// NOTE: schedule_atom is used instead of returning an atom
// to the C platform because it allows a non-pure guile layer
// to easily schedule multiple atoms as needed.
SCM schedule_atom(SCM scm_atom_desc, SCM parameters, SCM next) {
    // TODO add atom, tell garbage collector we care about SCM fields
    // (once atom collection is dynamically allocated)
    AtomDesc* atom_desc = scm_foreign_object_ref(scm_atom_desc, 0);
    if (s_atom_count < TEMP_ATOM_MAX) {
        Atom* atom = &s_atoms[s_atom_count];
        atom->is_complete = false;
        atom->value = SCM_UNDEFINED;
        atom->ro_parameters = SCM_UNDEFINED;
        atom->ro_next = next;
        atom_desc->init(atom, parameters);
        s_atom_count++;
    }
    return SCM_UNDEFINED;
}

void main_loop(SCM resume, SCM world_program) {

    // Process the world program and schedule first
    // atoms.
    scm_call_2(resume, world_program, SCM_UNDEFINED); 

    struct timespec t0;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        return;
    }

    while (true) {
        struct timespec t1;
        if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
            // TODO panic verbosely, or possibly better is
            // let things explode if the clock fails to remove
            // a branch
            break;
        }

        struct timespec elapsed = t1;
        subtract_time(&elapsed, &t0);
        t0 = t1;

        UpdateInfo update_info = {
            .elapsed = &elapsed,
            .interest_list = &s_interest_list,
        };
        update_atoms(s_atoms, s_atom_count, &update_info);

        // Find the min duration to wait for
        struct timespec* min_duration = NULL;
        for (uint32_t i = 0; i < s_atom_count;) {
            Atom* atom = &s_atoms[i];
            if (atom->is_complete) {
                // TODO test schedule from thread pool
                if (!scm_is_false(atom->ro_next)) {
                    scm_call_2(resume, atom->ro_next, atom->value);
                }

                // TODO free scm for completed atoms (when heap alloced)
                s_atom_count--;
                if (i < s_atom_count) {
                    s_atoms[i] = s_atoms[s_atom_count];
                    continue;
                }
            }
            else {
                // Atom still needs updating, find out when to wake up.
                if (atom->timer.tv_sec >= 0) {
                    // TODO make timespec lt function
                    if (min_duration == NULL ||
                        atom->timer.tv_sec < min_duration->tv_sec ||
                        (atom->timer.tv_sec == min_duration->tv_sec &&
                         atom->timer.tv_nsec < min_duration->tv_nsec)) {
                        min_duration = &atom->timer;
                    }
                }
            }
            i++;
        }

        if (s_atom_count > 0) {
            // Wake up after 1 min if nothing needs waking. TODO make configurable.
            // This could even be configured to never wait, and just sigwait with no
            // timeout.
            struct timespec default_timeout = {
                .tv_sec = 60, 
                .tv_nsec = 0
            };
            if (min_duration == NULL) {
                min_duration = &default_timeout;
            }
            printf("Sleeping for: %fs\n", (double)min_duration->tv_sec + (double)min_duration->tv_nsec / NSEC_PER_SEC);
            wait_interest_list(&s_interest_list, timespec_to_ms(min_duration));
        }
        else {
            break;
        }
    }
}

void* inner_main(void* data) {

    // NOTE: Thread pool seems to need guile initialized
    // before the worker threads can enter guile mode without
    // seg faulting.
    {
        if (init_thread_pool(&s_thread_pool) == false) {
            exit(EXIT_FAILURE);
        }

        // NOTE: We may have an option to turn multithreading off or
        // make it explicit, in which case we can make specific atoms
        // add or remove from the interest list based on threads with
        // queued work, instead of always listening to all of them.
        // That may improve epoll perf in the kernel but may not be worth the
        // extra management (needs profiling).
        // NOTE: If dynamically adding threads to thread pool then we should
        // also begin listening to them.
        for (size_t i = 0; i < s_thread_pool.thread_count; i++) {
            // Listen to the socket threads.
            add_interest(&s_interest_list, s_thread_pool.socket_pairs[i][0]);
        }


    }

    (void)data;

    // Foreign objects
    init_atom_desc_type();

    // register C functions:
    scm_c_define_gsubr("sleep-atom-desc", 0, 0, 0, sleep_atom_desc);
    scm_c_define_gsubr("read-atom-desc", 0, 0, 0, read_atom_desc);
    scm_c_define_gsubr("schedule-atom!", 3, 0, 0, schedule_atom);

    // TODO figure out how to get a variable defined when we load
    // a program? Can we define a main program and define a resume?
    // Or perhaps two files is best anyway.
    SCM resume = scm_c_primitive_load("src/resume.scm");
    SCM world_program = scm_c_primitive_load("src/world_program.scm");

    main_loop(resume, world_program);



    destroy_thread_pool(&s_thread_pool);

    return NULL;
}

// Next steps:
// - signal handlers, which can be implemented via
//   (listen_for_signal S) atom
// - multithreading via par and rpar, to start:
//   - choose thread from thread pool
//   - only perform the 'next' function on threads
//     or CPU bound atoms which are optimized to
//     thread-safe function calls: thinking more
//     about other options now in smirc_ideas_15
//   - what I was planning to start with is:
//     par as an atom.
// - rendering atoms
// - atoms as extensions
// - guile atoms(?)
// - write atom
// - generalize read/write atom to work across
//   different file descriptors like sockets
// - send & receive


int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // GRAPHICS
    // Eventually this should be configured by the guest language
    if (true) {
        Context context;
        if (!context_init(&context)) {
            puts(stfa_to_ascii[STFA_OUDENTHORPE]);
            exit(EXIT_FAILURE);
        }
        else {
            puts(stfa_to_ascii[STFA_MONOTHORPE]);
            const bool result = context_draw(&context);
            if (result) {
                context_sleep(&context, 600);
            }
            context_destroy(&context);
        }

    }


    // TODO pthread_sigmask might be what I want for multithreaded
    // programs.
    // TODO sigprocmask when we want to ignore signals sent to this
    // process, so that the filedescriptor can handle them.
    sigemptyset(&s_sig_set); // TODO update based on all scheduled atoms
    s_signal_fd = signalfd(-1, &s_sig_set, 0); // TODO error handle

    init_interest_list(&s_interest_list); // TODO error handle
    add_interest(&s_interest_list, s_signal_fd);

    // Set stdin to nonblocking
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK | fcntl(STDIN_FILENO, F_GETFL));

    // TODO clear interest list resources on shutdown?


    scm_with_guile(inner_main, NULL); // TODO add more data as needed

    return EXIT_SUCCESS;
}
