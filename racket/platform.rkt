#lang racket/base

(define scheduled-atoms '())

;; TODO if we need timeouts on certain sync results, then group an alarm-evt
;; with choice event

;; TODO try scribble for literate programming

;; WORLD PROGRAMS:
;; World programs are implemented as thunks which return
;; 0 or more synchronization events, representing continuing world effects
;; that must be performed by 'the platform'.

;; World programs are constructed through the composition of 'atomic' world
;; programs provided by the platform. These are known as atoms.

;; World programs can be composed via 'bind' or 'multi'.


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
      [else ((next wp-result))])))

;; TODO, just realized that choice would simplify this quite a bit and remove the
;; need for an atom list. Whether or not the solution performs well enough compared
;; to the original C design is to be seen, but it is more elegant. Try this
;; first.
(define (multi wps) 0)
 
;; TODO in this model these are not really atoms, but world program
;; continuations. The name should be changed. But I'm probably scrapping the
;; atom list entirely in favour of the single synchronization event model, at
;; least for now. If it needs to come back it will be for performance.
(struct atom (id event))
(struct atom-list (atoms free-ids))

;; Returns the next ID and next free ID list
(define (next-id/free-ids al)
  (define free-ids (atom-list-free-ids al))
  (define max-id (length (atom-list-atoms al)))
  (cond
    [(null? free-ids) (values max-id '())]
    [else (values (car free-ids) (cdr free-ids))]))

;; Returns an atom list with the given id removed
(define (remove-atom id al)
  (atom-list
   (filter (lambda (atom)
             (not (equal? (atom-id atom) id)))
           (atom-list-atoms al))
   (cons id (atom-list-free-ids al))))

(define empty-atom-list (atom-list '() '()))

(define (schedule-evt-as-atom evt al)
  (define-values (next-id next-free-ids)
    (next-id/free-ids al))
  (atom-list
   (cons (atom
          next-id
          (handle-evt
           evt
           (lambda (r)
             (atom-sync-result next-id r))))
         (atom-list-atoms al))
   next-free-ids))

;; Construct a list of all the sync objects
(define (atom-list-update al sr)
  ; Remove updated atom from the sync list
  (define event-id (atom-sync-result-id sr))
  (define event-original-result (atom-sync-result-original sr))
  (define remaining-atoms (remove-atom event-id al)) 
  (cond
    [(evt? event-original-result)
     ;; Schedule event if the synchronization result provides another
     ;; atom.
     (schedule-evt-as-atom event-original-result remaining-atoms)]
    ;; NOTE: 'multi' could be implemented by allowing WP thunks to return
    ;; multiple synchronization events, and we schedule each of them here.
    ;; I am migrating to use choice events instead for now.
    [else
     ;; Event has completed, nothing new to schedule.
     remaining-atoms]))

;; All atom sync objects are wrapped to return this struct, where the original
;; sync result is in 'original', but the id in the atom list is returned in 'id'
(struct atom-sync-result (id original))

;; Example wp to wait 2 seconds
(define wait-wp (wait-msec 2000))

(define (main wp)
  (define (main-loop al)
    (define evt-list
      (map (lambda (a) (atom-event a)) (atom-list-atoms al)))
    (cond
      [(null? evt-list) (display "DONE\n")]
      [else (main-loop (atom-list-update al (apply sync evt-list)))]))
  (main-loop (schedule-evt-as-atom (wp) empty-atom-list)))

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
;(main (bind wait-wp (lambda (test) (display "hello")(display test))))

;; Doesn't work:
;; as expected for now, since it never returns a sync object and
;; the main loop only continues when there are scheduled sync objects and will complain
;; if not handed a wp that returns a sync object.
; (main (bind current-ms-atom (lambda (ms) current-ms-atom)))

; Works:
(main (bind current-ms-atom (lambda (ms) wait-wp)))

;; TODO for next commit: make an only sync event main loop, remove atom list
