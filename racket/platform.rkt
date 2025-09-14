#lang racket/base

(define scheduled-atoms '())

;; TODO if we need timeouts on certain sync results, then group an alarm-evt
;; with choice event

;; TODO try scribble for literate programming

;; WORLD PROGRAMS:
;; World programs are programs which interact with the world to perform
;; effects and retrieve information.

;; World programs are implemented as thunks which return a synchronization
;; event, which is a promise to return a value or interact with the world
;; at a later time, or they may return immediately with a value.

;; World programs are constructed through the composition of 'atomic' world
;; programs provided by the platform. These are known as atoms.

;; World programs can be composed via 'bind' or 'multi' into larger
;; world programs.


;; ATOM IMPLEMENTATIONS
(define (wait-msec msec)
  (lambda ()
    ;; Use monotonic with #t
    (handle-evt
     (alarm-evt (+ (current-inexact-monotonic-milliseconds) msec) #t)
     ;; Don't return the alrm; we don't want to sync on it again
     (lambda (alrm) (void))))) 


;; WP COMBINATORS

;; Create a new world program from wp, that feeds the result of wp into
;; a pure function, next, which should also return a world program.
(define (bind wp next)
  (lambda ()
    ;; If wp returns a sync object (promise) then, return a promise that returns
    ;; the result of calling next on the promise's result

    ;; If wp is a known value, then return the result of next when passed the
    ;; known value

    ;; This allows atoms to be implemented as plain functions that have
    ;; immediate effects, for times where no real wait is expected. This way we
    ;; circumvent waiting for the main loop to synchronize on a quick atom, and
    ;; just jump into that atom's implementation immediately.
    (define wp-result (wp))
    (cond
      [(evt? wp-result) (handle-evt wp-result (lambda (sr) (next sr)))]
      [else ((next wp-result))]))) ;; TODO: could add a guard to prevent infinite recursion here, forcing scheduling via always event

;; TODO: create a lambda that runs each of the wps, and for each of the sync events
;; that they return (handle the non sync event case as well), we create a choice
;; that will sync on any of them. When any of them wake, they need to be wrapped in a handle-event
;; that will return a multi for all of the rest. They could capture

;; THe actual strategy will be a handle event wrapping a choice evenet wrapping handle events for all of the sync events generated
;; by each of the WPs. The outer handle event reconstructs itself but replaces the event with it's syncrhonization result (if it is
;; a new event) in the list, keeping the others.

;; This isn't any better or more elegant than having the main loop handle lists of synchronization objects, because that is
;; exactly what we're doing here but confining it to multi. It does maintain program structure in some way and I wonder if
;; that will be helpful for debugging. I wonder if the overhead of extra wrapping events will be a problem, or non
;; locality of a vector of events will be a problem. But I can always change the scheduler later to optimize if needed.
(define (multi wps)
  ;; Run the world programs for a list of just the synchronization events
  (define events (filter evt? (map (lambda (fn) (fn)) wps)))
  (multi-event events))

(define (multi-event events)
  ;; Map the events into events that return their sync result plus the event it came from
  (define events_with_id ;; TODO change name!
    (map (lambda (evt)
           (handle-evt evt (lambda (res)
                             (cons res evt)))) events))

  ;; Choice will sync on any of the events in events-with-id
  (define choice (apply choice-evt events_with_id))

  ;; Make a sync event that will create a new multi based on the returned value from
  ;; choice. It may be continuing computation as requested by a sync object or terminated
  ;; as indicated by a non evt? value, in which case we only need to track the remaining
  ;; events that were not synced yet. In the former, we need to wait for those events plus
  ;; the new one returned.
  (handle-evt choice (lambda (event_with_id)
                       (define events_less_this_one (remove (cdr event_with_id) events)) ; TODO change name!
                       (define sync-result (car event_with_id))
                       (cond
                         [(evt? sync-result) (multi-event (cons sync-result events_less_this_one))]
                         [else (multi-event events_less_this_one)]))))
 
;; Example wp to wait 2 seconds
(define wait-wp (wait-msec 2000))

(define (main wp)
  (define (main-loop cur)
    (cond
      [(evt? cur) (main-loop (sync cur))]
      [else (display "WORLD PROGRAM TERMINATED:") (display cur) (newline)]))
  (main-loop (wp)))

;; TODO: starvation if you have a WP that returns an object directly,
;; bound to itself. If we're allowing direct function calls for some atom
;; implementations, e.g. current-ms, to avoid scheduling delays, then you
;; also avoid any chance at time sharing via the pseudo random sync result
;; returned by any ready synchronization events.

;; Of course this isn't a reasonable program, but you could make such as mistake
;; during development and lock your interactive program instead of slowing it
;; down, this could be a problem if you are in an editor program that involves
;; running the programs you are writing, and you have unsaved progress. A monitor
;; thread might be needed to recover or detect these problems or we can do more
;; analysis than present to do maybe-locking-loop detection, and use a scheduling
;; model instead of an instant model for feeding WP effects to the next function
;; of a bind.

;; Or, we can just always use the scheduling model instead of an instant feed,
;; maybe that's good enough, but that remains to be seen. If there are program
;; atoms that are used extremely frequently in an interactive program than avoiding
;; scheduling overhead would be important.

;; example:
(define current-ms-atom
  (lambda ()
    (current-milliseconds)))

(define looping-wp
  (bind current-ms-atom (lambda (ms) (display "GOTMS\n")looping-wp)))

;; Looping-wp is a lambda that will never return, it is permanently
;; jumping to looping-wp when it gets executed because the next of
;; it's bind returns the bind lambda itself, and we call the result
;; of the next function (which returns a wp, in this case looping-wp)
;; so we call the bind lambda when we run the bind lambda, looping
;; with tail recursion indefinitely.
;; This is expected because the 'optimization' of subverting the scheduler
;; means that we call the wp generated by 'next' in bind itself, if the
;; value to give to next is ready right away. In a useful program the
;; call to next should eventually accomplish something by evaluating a wp
;; that does something useful.

;; Thinking more about this, if we allow this optimization to avoid the scheduler,
;; then you could write 'valid' world programs that just end up like loops, e.g.
;; the program that constantly prints out the time without ever synchronizing,
;; assuming you have an 'optimized' output function that doesn't use sync objects
;; and just immediately outputs.

;; The trouble with not synchronizing is that if you wanted to combine such a program
;; with anything else, it would get completely starved by the scheduler that never
;; gets to run.

;; We can either 'just deal with it' which is lame if you find yourself locking your
;; interactive program up, or remove the optimization so all atoms are sync objects,
;; even if that means using sync objects that are always ready for syncrhonization
;; (that is a pretty elegant solution IMO and hopefully it isn't slow with a good
;; selection of atoms at your disposal), or we could do some kind of monitoring that
;; chooses to kick in the scheduler dynamically so you can write a silly program
;; that loops to itself, but the optimization effectively gets dynamically disabled.
;; You could possibly make all the optimized atoms track a call depth that scheduling
;; resets, and once it hits some number you return a synch object allowing something
;; else to run. That way, nothing ever gets starved forever and the random
;; selector will do it's job, and all the non-silly programs will still benefit from
;; the optimization.


; (main (bind looping-wp (lambda (_something) wait-wp)))

;; Example bind tests that works
(main (bind wait-wp (lambda (test) (display "hello"))))

;; Doesn't work:
;; as expected for now, since it never returns a sync object and
;; the main loop only continues when there are scheduled sync objects and will complain
;; if not handed a wp that returns a sync object.
; (main (bind current-ms-atom (lambda (ms) current-ms-atom)))

; Works:
;(main (bind current-ms-atom (lambda (ms) wait-wp)))

;; TODO for next: try a call depth limitation for world programs that never
;; go to scheduling, forcing them to schedule instead of calling next.
;; Is there some kind of monad I can create for this purpose? The monadic context
;; around the computation would be the number of calls (since last scheduling
;; opportunity). We would want to make functions that just return plain
;; values, but lift them (say during a bind OR when first entering main loop)
;; into a monadic context after scheduling has begun. That monadic context
;; would track call depth, and return a sync event that could be always on or
;; could be delayed by an alarm evt. Probably always on is better, let hte random
;; number generator deal with scheduling problems and not artificially delay
;; some programs.
