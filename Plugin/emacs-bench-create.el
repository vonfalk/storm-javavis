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

    ;; For the final evaluation:
    (edit-incomplete "edit" bench-create-incomplete)
    (edit-random "edit" bench-create-random)
    (edit-scope "edit" bench-create-scope)
    (edit-string "edit" bench-create-string)
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

(defun insert-edit-cmd (from to text)
  (setq text (substring-no-properties text))
  (let* ((print-escape-newlines t)
	 (format-str "// (edit %5d %5d %S)\n")
	 (header (format format-str from to text)))

    ;; Adjust offsets for the inserted header.
    (setq from (+ from -1 (length header)))
    (setq to   (+ to   -1 (length header)))

    ;; Generate the header once more and insert it!
    (goto-char (point-min))
    (insert (format format-str from to text))))

(defun bench-create-incomplete ()
  "Create incomplete constructs, similarly to 'bench-create-partial-insert', but using the 'edit' mode instead."
  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let* ((min-pos (1+ (point)))
	 (regex "\\(^.*; *$\\)\\|\\(^ *\\(if\\|for\\).*{$\\)")
	 (match-count (bench-count-matches regex min-pos))
	 begin end text add-newline)
    (bench-goto-match regex (random match-count) min-pos)

    (setq end (point))
    (beginning-of-line)
    (skip-chars-forward " \t")
    (setq begin (point))
    (setq text (buffer-substring begin end))

    (if (equal "{" (substring text -1))
	(progn
	  ;; Don't remove anything, insert a duplicate of this statement instead.
	  (beginning-of-line)
	  (setq begin (point))
	  (setq text (buffer-substring begin end))
	  (setq add-newline t)
	  (setq end begin))
      (progn
	(delete-region begin end)))

    ;; Randomly shrink the string:
    (let ((chars 1))
      (setq text (substring text 0 (+ (random (- (length text) chars)) chars))))
    (when add-newline
      (setq text (concat text "\n")))

    ;; Insert the header
    (insert-edit-cmd begin begin text)
  ))

(defun bench-create-random ()
  "Insert or remove random tokens or symbols in the source."
  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let ((min-pos (1+ (point)))
	found begin end text)

    (while (not found)
      ;; Pick a random location
      (goto-char (+ min-pos (random (- (point-max) min-pos))))
      (skip-chars-forward " \n\r\t")

      (unless (or (inside-comment-p)
		  (inside-string-p)
		  (string-prefix-p "/*" (buffer-substring (point) (+ 2 (point)))))
	;; Remove the token nearby...
	(setq found (extract-word))))

    (setq begin (first found))
    (setq end (second found))
    ;; (setq text (buffer-substring begin end))
    (insert-edit-cmd begin end "")
    ))

(defun extract-word ()
  (let ((start (point))
	(curr (char-syntax (string-to-char (buffer-substring (point) (1+ (point))))))
	beg end)

    (setq beg start)
    (while (and (> beg (point-min))
		(= curr (char-syntax (string-to-char (buffer-substring (1- beg) beg)))))
      (setq beg (1- beg)))

    (setq end start)
    (while (and (< end (point-max))
		(= curr (char-syntax (string-to-char (buffer-substring end (1+ end))))))
      (setq end (1+ end)))

    (message "%S %S" curr (buffer-substring-no-properties beg end))
    (list beg end)))

(defun inside-string-p ()
  "Determines if the cursor is inside a string."
  (nth 3 (syntax-ppss)))

(defun inside-comment-p ()
  "Determines if the cursor is inside a comment."
  (nth 4 (syntax-ppss)))

(defun bench-create-scope ()
  "Introduce scoping errors by adding { or } symbols."
  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let* ((min-pos (1+ (point)))
	 (regex "[{};] *$")
	 (match-count (bench-count-matches regex min-pos)))
    (bench-goto-match regex (random match-count) min-pos))

  (end-of-line)
  (insert-edit-cmd (point) (point) (if (= 0 (random 2)) "{" "}"))
  )

(defun bench-create-string ()
  "Introduce non-closed string literals or comment blocks."
  (interactive)

  (goto-char (point-min))
  (search-forward-regexp "^package.*; *$" nil t)

  (let ((min-pos (1+ (point)))
	found)
    (while (not found)
      ;; Pick a random location
      (goto-char (+ min-pos (random (- (point-max) min-pos))))
      (skip-chars-forward " \n\r\t")

      (unless (inside-comment-p)
	;; Insert a string or a comment here...
	(insert-edit-cmd (point) (point) (if (= 0 (random 2)) " \"str " " /* "))
	(setq found t))
      )))

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


;; Create tests for the final benchmark...
(when nil
  (dolist (elem '((edit-incomplete "incomplete/" 200)
		  (edit-random "random/" 200)
		  (edit-scope "scope/" 100)
		  (edit-string "string/" 100)))
		  ))
    (bench-create "~/Projects/exjobb/data/all/"
		  (concat "~/Projects/exjobb/data/" (second elem))
		  (first elem)
		  (third elem)))
  )
