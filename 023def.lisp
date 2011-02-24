(setf wart-signatures* (table))

(defmacro$ def-case(name params test &body body)
  `(progn
    (push (list (fn args
                  (and (apply (length-matcher ',params) args)
                       (apply (fn ,params ,test) args)))
                (fn ,params
                  (handling-$vars ,@body)))
          (gethash ',name wart-signatures*))
    (defun$ ,name(&rest ,$args)
      (call-correct-variant (gethash ',name wart-signatures*)
                            ,$args))))

(defmacro$ def-normal(name params &body body)
  `(progn
    (push (list (length-matcher ',params)
                (fn ,params
                  (handling-$vars ,@body)))
          (gethash ',name wart-signatures*))
    (defun$ ,name(&rest ,$args)
      (call-correct-variant (gethash ',name wart-signatures*)
                            ,$args))))

(defmacro$ def(name params &body body)
  (case (car body)
    (:case  `(def-case ,name ,params ,(cadr body) ,@(cddr body)))
    (otherwise `(def-normal ,name ,params ,@body))))

(defmacro proc(name args . body)
  `(def ,name ,args ,@body nil))



;; Internals

(defun call-correct-variant(variants args)
  (if variants
    (destructuring-bind (test func) (car variants)
      (if (or (singlep variants)
              (apply test args))
        (apply func args)
        (call-correct-variant (cdr variants) args)))))

(defun length-matcher(params)
  (fn args
    (>= (length (remove-if [kwargp params _] args)) ; len will be overridden
        (length (required-params params)))))
