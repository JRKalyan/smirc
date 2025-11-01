#lang racket/base

(require racket/future
         racket/place)

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


;; TODO consider a platform which learns when to schedule work on another
;; thread and when to simply call the pure work.
(define (schedule-pure-work work)
  (define worker-ch (make-channel))
  (define worker-thread (thread (lambda () (channel-put worker-ch (work)))))
  worker-ch)

;; Create a new world program from wp, that feeds the result of wp into
;; a pure function, next, which should also return a world program.
(define (bind wp next)
  (lambda ()
    (define wp-result (wp))
    (cond
      [(evt? wp-result)
       ;; When the sync event is ready, schedule the next function to run with its result
       (handle-evt wp-result (lambda (sr) (schedule-pure-work (lambda () (next sr)))))]
      [else
       ;; The next value is ready now, so schedule the next function to run with the result
       (schedule-pure-work (lambda () (next wp-result)))])))

(define (multi wps)
  (lambda ()
    ;; Run the world programs for a list of just the synchronization events
    (define events (filter evt? (map (lambda (fn) (fn)) wps)))
    (multi-event events)))

(define (multi-event events)
  (cond
    [(null? events) (void)]
    [(null? (cdr events)) (car events)]
    [else
     ;; Map the events into (handle) events that return their sync result plus
     ;; the event it came from, and make a choice event that tracks each of
     ;; those. A final handle event will create a multi event for the remaining
     ;; sync events.
     (let* ([event-with-id (map
                            (lambda (evt)
                              (handle-evt evt (lambda (res) (cons res evt))))
                            events)]
            [choice (apply choice-evt event-with-id)])
       ;; Make a sync event that will create a new multi based on the returned
       ;; value from choice. It may be continuing computation as requested by a
       ;; sync object or terminated as indicated by a non evt? value, in which
       ;; case we only need to track the remaining events that were not synced
       ;; yet. In the former, we need to wait for those events plus the new one
       ;; returned.
       (handle-evt choice
                   (lambda (event-with-id)
                     (define remaining-events (remove (cdr event-with-id) events))
                     (define sync-result (car event-with-id))
                     (cond
                       [(evt? sync-result) (multi-event (cons sync-result remaining-events))]
                       [else (multi-event remaining-events)]))))]))
 
;; Example wp to wait 2 seconds
(define wait-wp (wait-msec 2000))

(define (main wp)
  (define (main-loop cur)
    (cond
      [(evt? cur) (main-loop (sync cur))]
      [else (display "WORLD PROGRAM TERMINATED:") (display cur) (newline)]))
  (main-loop (wp)))

;; example:
(define current-ms-atom current-milliseconds)

(define looping-wp
  (bind current-ms-atom (lambda (ms) (display "GOTMS\n")looping-wp)))

(define (display-atom v)
  (lambda () (display v)))

(define do-two-wp (multi (list (display-atom "HELLO1") (display-atom "bello2"))))

(define simple-bind (bind wait-wp (lambda (v) (display-atom "Goodbye"))))
(define do-three-wp (multi (list (display-atom "Hi") simple-bind)))

;; NOTE since display-atom takes 1 arg, we can do this:
(define simple-bind2 (bind wait-wp display-atom))

;(main (bind wait-wp (lambda (test) (display "hello"))))
;(main (bind wait-wp do-two-wp))
;(main do-two-wp)
;(main do-three-wp)
;(main simple-bind2)
