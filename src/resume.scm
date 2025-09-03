;; A world program is a scm object (TBD format) that
;; represents a computation to be performed in the context
;; of an outer world, with side effects.
(define (do-world-program wp next)
  (cond
   ;; If bind, then the first element of wp is 'bind.
   ;; the second element (cadr) is the sub world program that produces a value v
   ;; the third element (caddr) is a function that takes a value v, and produces a monad.
   [(eq? 'bind (car wp)) (do-world-program
                          (cadr wp)
                          (cond
                           [(eq? #f next) (caddr wp)]
                           [else (lambda (v) `(bind ,((caddr wp) v) ,next))]))]
   [else (schedule-atom! (car wp) (cdr wp) next)])) ; first param is an atom(desc), second is parameters, and third is for next

;; Multithreading support:
;; if we are resuming on a child thread, then we don't call schedule-atom! directly, but
;; instead write to the socket and provide a thunk for the main thread to execute.
;; The main thread will epoll for thread signals.

;; TODO multi support, schedule multiple atoms at once.

;; TODO parallel computations:
;; could possibly just handle a 'multibind thing
;; which calls do-world-program on every thing in the list.

;; f is a function that when given value, returns a world program
;;(lambda (f value)
;;  (with-exception-handler
;;   (lambda (exn)
;;     (display "Oh no!")
;;     (newline)
;;     #f)
;;   (lambda () (step-world-program (f value)))))

(lambda (f value) (do-world-program (f value) #f))
