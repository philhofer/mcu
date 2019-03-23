;; sharp-string-syntax

(module sharp-string (expand-quoted)
(import
  scheme
  srfi-1
  chicken.base
  chicken.syntax
  chicken.read-syntax
  chicken.format
  chicken.port
  chicken.io)

;; expand-quoted handles reading
;; the #"string: #(foo bar)" syntax
;; and expands it into an equivalent
;; s-expression
(define (expand-quoted port)
  ; the input string is delimited
  ; on the trailing " and intermediate
  ; # symbols
  (define (delimit? c)
    (not (or (equal? c #\")
	     (equal? c #\#))))
  (define (nonempty? str)
    (not (and (string? str)
	      (equal? str ""))))
  (define (output-exprs exprs)
    (define writes (map (lambda (expr)
			  `(display ,expr))
			(reverse (filter nonempty? exprs))))
    (if (eq? (length writes) 0)
	'""
	`(with-output-to-string
	   (lambda ()
	     ,@writes))))

  (define (err msg)
    (let-values ([(line col) (port-position port)])
      (syntax-error (format "line ~a col ~a: ~a" line col msg))))

  (define (read-item port brace)
    (when brace
      (or (eqv? (read-char port) #\{)
	  (err "expected leading {")))
    (let ([item (read port)])
      (when brace 
	(or (eqv? (read-char port) #\})
	    (err "expected trailing }")))
      item))

  (let loop ([exprs '()])
    (let ([cs (read-token delimit? port)]
	  [nc (read-char port)])
      (if (equal? nc #\")
	  (output-exprs (if (equal? cs "") ; done!
			    exprs
			    (cons cs exprs)))
	  (loop (cons
		  (read-item port (eqv? #\{ (peek-char port)))
		  (cons cs exprs)))))))

#;(define (expand-quoted-debug port)
  (let ([out (expand-quoted port)])
    (write out)
    (display "\n")
    out))

  (set-sharp-read-syntax! #\" expand-quoted)

)
