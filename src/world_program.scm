(use-modules (scheme base))
;; Loop sleep1000 by providing an f that produces sleep1000 (given nothing from sleep atom)
(define sleep1000 (cons (sleep-atom-desc) 1000))
;(define mynext (lambda (value) `(bind ,sleep1000 ,mynext)))
;mynext

(define getbytes (cons (read-atom-desc) 4))
(define printval (lambda (value) (display "You said: \n") (display (utf8->string value)) (newline) sleep1000))
(lambda (value) `(bind ,getbytes ,printval))
