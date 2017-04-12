;; Creation of random benchmark tests from a given set of source files.

(defun bench-create (input output type count)
  "Use files in 'input' to create a set of benchmark tests in 'output', tests will be of the type 'type'."

  (setq input (bench-find-files (file-name-as-directory input)))
  (setq output (file-name-as-directory output))
  (setq type-info (assoc type bench-types))
  (when (not type-info)
    (user-error "Type %S not a known benchmark type" type))
  (if (file-exists-p output)
      (bench-clear-directory output)
    (make-directory output t))
  (bench-create-files input output type-info count))

(defvar bench-types
  '(
    (insert-whole "insert" bench-create-whole-insert)
    (insert-partial "insert" bench-create-partial-insert)
    )
  "Known types of tests to create. Each entry has the form: type file-ext function
   where 'type' is the key used in lookups, 'file-ext' is the file extension to be
   used and 'function' is the function that actually creates the test.")

(defun bench-clear-directory (dir)
  (let ((files (directory-files-and-attributes dir)))
    (while (consp files)
      (let* ((file (first files))
	     (name (car file))
	     (isdir (first (cdr file))))
	(unless isdir
	  (delete-file (concat dir name))))
      (setq files (rest files)))))

(defun bench-find-files (dir)
  (let ((files (directory-files-and-attributes dir))
	(result nil))
    (while (consp files)
      (let* ((file (first files))
	     (name (car file))
	     (attrs (cdr file)))
	(when (and (not (= (string-to-char name) ?.))
		   (not (= (string-to-char name) ?#))
		   (not (eq (first attrs) t)))
	  (setq result (cons (concat dir name) result))))
      (setq files (rest files)))
    result))

(defun random-element (list)
  (nth (random (length list)) list))

(defun bench-create-files (input output type count)
  "Create tests for files in the list 'input'."
  (while (> count 0)
    (bench-create-file (random-element input) output type)
    (setq count (1- count))))

(defun bench-generate-name (input type seq)
  (format "%s-%03d.%s.%s" (file-name-base input) seq (second type) (file-name-extension input)))

(defun bench-find-name (input output type)
  (let ((seq 0))
    (while (file-exists-p (concat output (bench-generate-name input type seq)))
      (setq seq (1+ seq)))
    (concat output (bench-generate-name input type seq))))

(defun bench-create-file (input output type)
  "Create a test for the file in 'input'."
  (let ((name (bench-find-name input output type)))
    (message "Creating file %s..." name)
    (redisplay)
    (with-temp-buffer
      (insert-file-contents input)
      (funcall (third type))
      (write-file name))))

(defun bench-random-pos (&optional min-pos)
  (unless min-pos
    (setq min-pos (point-min)))
  (goto-char (+ min-pos (random (- (point-max) min-pos)))))

(defun bench-count-matches (regex &optional min)
  (goto-char (if min min (point-min)))
  (let ((count 0))
    (while (search-forward-regexp regex nil t)
      (setq count (1+ count)))
    count))

(defun bench-goto-match (regex id &optional min)
  (goto-char (if min min (point-min)))
  (let ((count 0))
    (while (<= count id)
      (search-forward-regexp regex nil t)
      (setq count (1+ count)))))

(defun bench-create-whole-insert ()
  "Create a test which inserts a whole statement somewhere in the current buffer."
  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let* ((min-pos (1+ (point)))
	 (regex "^.*; *$")
	 (match-count (bench-count-matches regex min-pos)))
    (bench-goto-match regex (random match-count) min-pos))

  (let ((end (point))
	(header nil))
    (beginning-of-line)
    (skip-chars-forward " \t")
    (setq header (buffer-substring (point) end))
    (delete-region (point) end)
    (insert "$$")
    (goto-char (point-min))
    (insert "// $$\n// " header "\n")))

(defun bench-create-partial-insert ()
  "Create a test which insert a partial statement somewhere in the current buffer."
  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let* ((min-pos (1+ (point)))
	 (regex "\\(^.*; *$\\)\\|\\(^ *\\(if\\|for\\).*{$\\)")
	 (match-count (bench-count-matches regex min-pos))
	 (end nil)
	 (point nil))
    (bench-goto-match regex (random match-count) min-pos)

    (setq end (point))
    (beginning-of-line)
    (skip-chars-forward " \t")
    (setq header (buffer-substring (point) end))
    (when (equal "{" (substring header -1))
	;; Remove the next line instead...
	(search-forward-regexp "^.*; *$" nil t)
	(setq end (point))
	(beginning-of-line)
	(skip-chars-forward " \t"))
    (delete-region (point) end)
    (insert "$$")
    (goto-char (point-min))

    (let ((chars 4))
      (setq header (substring header 0 (+ (random (- (length header) chars)) chars))))
    (insert "// $$\n// " header "\n")))


;; Create insert-whole tests:
;; (bench-create "~/Projects/storm/root/test/server-tests/ant/ref/"
;; 	      "~/Projects/storm/root/test/server-tests/ant/insert/"
;; 	      'insert-whole
;; 	      200)

;; (storm-run-benchmarks "test/server-tests/ant/insert/")


;; Create insert-partial tests:
;; (bench-create "~/Projects/storm/root/test/server-tests/ant/ref/"
;; 	      "~/Projects/storm/root/test/server-tests/ant/partial/"
;; 	      'insert-partial
;; 	      200)

;; (storm-run-benchmarks "test/server-tests/ant/partial/")

