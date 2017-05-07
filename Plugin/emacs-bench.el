;; Benchmark tests for the language server.

;; Run tests using C-c b
(global-set-key (kbd "C-c b") 'storm-run-benchmarks)
(global-set-key (kbd "C-c c b") 'storm-run-benchmark-file)
(global-set-key (kbd "C-c C-c b") 'storm-run-benchmark-file)

;; Test root directory.
(defvar benchmark-root-dir "test/server-tests/simple/" "Benchmarks root directory, relative to 'storm-mode-root'.")
(defvar bench-batch-run nil "Run in batch mode, producing either 'error or 'time data.")
(defvar bench-incr-buffer "bench-incr.result" "Name of the buffer holding incremental results.")
(defvar bench-raw-buffer "bench-raw.result" "Name of the buffer holding raw results.")

(defmacro storm-test-env (&rest body)
  `(let ((old-storm-mode global-storm-mode)
	 (old-chunk-size (storm-query '(chunk-size))))
     (setq global-storm-mode nil)
     (storm-send '(chunk-size 0))

     (when bench-batch-run
       (get-buffer-create bench-raw-buffer)
       (get-buffer-create bench-incr-buffer))

     (prog1
	 ,@body
       (setq global-storm-mode old-storm-mode)
       (storm-send (cons 'chunk-size old-chunk-size)))))

;; Run benchmarks.
(defun storm-run-benchmarks (&optional dir)
  "Run benchmarks. Make sure Storm is running before starting the benchmarks."
  (interactive)
  (unless dir
    (setq dir benchmark-root-dir))
  (storm-test-env
    (storm-run-dir (concat (file-name-as-directory storm-mode-root) dir))))

(defun storm-run-benchmark-file ()
  (interactive)
  (when buffer-file-name
    (storm-test-env
     (storm-run-file (file-name-directory buffer-file-name) (file-name-nondirectory buffer-file-name) (cons 0 nil)))))

(defun storm-run-dir (dir &optional counter)
  "Run benchmarks in a directory."
  (unless counter
    (setq counter (cons 0 nil)))
  (setq dir (file-name-as-directory dir))
  (let ((files (directory-files-and-attributes dir)))
    (while (consp files)
      (storm-run-file-or-dir
       dir
       (car (car files))
       (cdr (car files))
       counter)
      (setq files (cdr files)))))

(defun storm-run-file-or-dir (path file attrs counter)
  "Run the test specified in the file."
  (cond ((= (string-to-char file) ?.)
	 nil)
	((= (string-to-char file) ?#)
	 nil)
	((eq (first attrs) 't)
	 (storm-run-dir (concat path file) counter))
	(t (storm-run-file path file counter))))

(defun storm-run-file (path file counter)
  "Run the specified file (if supported)."
  (let ((dispatch-to nil))
    (when (storm-supports (file-name-extension file))
      (setq dispatch-to (storm-find-type file)))

    (if dispatch-to
	(catch 'bench-fail
	  (setcar counter (1+ (car counter)))
	  (storm-output-string (format "Running file %d: %s...\n" (car counter) file) 'storm-bench-msg)
	  (funcall dispatch-to (concat path file)))
      (storm-output-string (format "Ignoring unsupported file: %s%s\n" path file)))))

(defun storm-find-type (file)
  (let ((at storm-bench-types)
	(file-kind (file-name-extension (file-name-sans-extension file)))
	(result nil))
    (while (consp at)
      (let ((kind (car (first at)))
	    (to   (cdr (first at))))
	(setq at (rest at))
	(when (equal kind file-kind)
	  (setq result to)
	  (setq at nil))
	))
    result))

(defun kill-current-mode ()
  "Disable the current mode and go back to text-mode."
  (kill-all-local-variables)
  (text-mode))

(defun storm-open-buffer (file use-storm-mode)
  "Open a buffer, wait until it is fully colored and ready for use."
  (save-excursion
    (let ((buf (find-buffer-visiting file))
	  (opened nil))
      (unless buf
	(setq buf (find-file-noselect file))
	(setq opened t))
      (display-buffer buf t)
      (with-current-buffer buf
	(if use-storm-mode
	    (progn
	      (when (buffer-modified-p)
		(kill-current-mode)
		(revert-buffer t t))
	      (storm-mode)
	      (storm-update-colors))
	  (progn
	    (kill-current-mode)
	    (when (buffer-modified-p)
	      (revert-buffer t t)))))
      (cons buf opened))))

(defun storm-update-colors ()
  "Update colors in the current buffer, wait for the result from Storm."
  (storm-send (list 'color storm-buffer-id))
  (storm-wait-for 'color 5.0))

(defface storm-bench-msg
  '((t :foreground "dark green"))
  "Face used indicating test status.")

(defface storm-bench-fail
  '((t :foreground "red"))
  "Face used indicating test failures.")

(defface storm-hilight-diff-face
  '((t :background "red"))
  "Face used to indicate erroneous syntax highlighting.")

(defun storm-compare-buffer-part (ref-str buf-start str-start length)
  (let ((str-pos str-start)
	(buf-pos buf-start)
	(count 0)
	(errors 0))
    (while (< count length)
      (let ((cur (get-text-property buf-pos 'font-lock-face))
	    (ref (get-text-property str-pos 'font-lock-face ref-str)))
	(unless (equal cur ref)
	  (with-silent-modifications
	    (put-text-property buf-pos (1+ buf-pos) 'font-lock-face 'storm-hilight-diff-face))
	  (unless (is-whitespace (aref ref-str str-pos))
	    (setq errors (1+ errors)))))
      (setq str-pos (1+ str-pos))
      (setq buf-pos (1+ buf-pos))
      (setq count   (1+ count)))
    errors))

(defun is-whitespace (char)
  (let ((ws " \n\r\t"))
    (or (= (aref ws 0) char)
	(= (aref ws 1) char)
	(= (aref ws 2) char)
	(= (aref ws 3) char))))

(defun storm-compare-buffer (file ref-str &optional ignore-from ignore-to)
  (unless (numberp ignore-from)
    (setq ignore-from 0))
  (unless (numberp ignore-to)
    (setq ignore-to 0))

  (let ((str-pos 0)
	(buf-pos (point-min))
	(errors 0))
    (while (< buf-pos (point-max))
      (when (or (<  buf-pos ignore-from)
		(>= buf-pos ignore-to))
	(let ((cur (get-text-property str-pos 'font-lock-face ref-str))
	      (ref (get-text-property buf-pos 'font-lock-face)))
	  (unless (equal cur ref)
	    (with-silent-modifications
	      (put-text-property buf-pos (1+ buf-pos) 'font-lock-face 'storm-hilight-diff-face))
	    (setq errors (1+ errors))))
	(setq str-pos (1+ str-pos)))
      (setq buf-pos (1+ buf-pos)))

    (when (> errors 0)
      (storm-output-string (format "%S errors in %s\n" errors file) 'storm-bench-fail))
    errors))

(defun storm-insert-slowly (text)
  "Insert the string 'text' at point, one character at a time."
  (let ((pos 0))
    (while (< pos (length text))
      (insert (substring-no-properties text pos (1+ pos)))
      (storm-query '(test echo))
      (redisplay)
      (setq pos (1+ pos)))))


(defvar storm-bench-types
  '(
    ("fill" . storm-run-fill)
    ("insert" . storm-run-insert)
    ("edit" . storm-run-edit)
    )
  "Kinds of tests that are currently supported.")

(defmacro storm-use-buffer (file auto-storm-mode &rest body)
  "Opens 'file' and make it visible, then evaluates all forms in 'body'.
   If the last form returns a nonzero value, the buffer is left in its
   current state, otherwise it is reverted (and maybe closed)."
  `(let* ((buffer-data (storm-open-buffer ,file ,auto-storm-mode))
	  (result (with-current-buffer (car buffer-data)
		    (auto-save-mode -1) ;; No auto-save-mode
		    ,@body)))
     (when (= 0 result)
       (with-current-buffer (car buffer-data)
	   (revert-buffer t t)
	   (when (cdr buffer-data)
	     (kill-buffer (car buffer-data)))))
     result))

(defun storm-run-fill (file)
  "Run files with .fill.X."
  (storm-use-buffer
   file t
   (let ((ref (buffer-substring (point-min) (point-max))))
     (erase-buffer)
     (storm-wait-for 'color 1.0)
     ;; Re-open the current buffer, so we start with a clean slate!
     (storm-debug-re-open)
     (storm-update-colors)

     (storm-insert-slowly ref)
     (storm-update-colors)

     (set-buffer-modified-p nil)
     (redisplay)
     (storm-compare-buffer file ref))))

(defun storm-run-insert (file)
  "Run files with .insert.X."
  (storm-use-buffer
   file nil
   (let* ((header (storm-insert-header))
	  (insert-str (first header))
	  (insert-text (rest header))
	  (ref-str "")
	  (gap-start 0))
     (unless (search-forward insert-str nil t)
       (let ((msg (format "The marker %S was not found in the file.\n" insert-str)))
	 (storm-output-string msg 'storm-bench-fail)
	 (throw 'bench-fail msg)))

     (delete-char (- (length insert-str)))
     (storm-mode)
     (storm-update-colors)
     (redisplay)

     (setq ref-str (buffer-substring (point-min) (point-max)))
     (setq gap-start (point))

     (while insert-text
       (storm-insert-slowly (car insert-text))
       (setq insert-text (cdr insert-text))
       (when insert-text
	 (insert "\n")
	 (indent-according-to-mode))
       (storm-update-colors)
       (redisplay))

     (storm-compare-buffer file ref-str gap-start (point)))))

(defun storm-insert-header ()
  "Extract the header lines for an 'insert'-type file."
  (goto-char (point-min))
  (let* ((current-line (next-buffer-line))
	 (look-for (concat (first (split-string current-line " ")) " "))
	 (result '()))
    (while (string-prefix-p look-for current-line)
      (setq result (append result (list (substring current-line (length look-for)))))
      (setq current-line (next-buffer-line)))
    result))

(defun next-buffer-line ()
  "Get the next line from a buffer."
  (prog1
      (buffer-substring-no-properties (line-beginning-position) (line-end-position))
    (goto-char (line-beginning-position 2))))


(defun storm-run-edit (file)
  "Run files with .edit.X."
  (storm-use-buffer
   file t
   (let* ((ref  (buffer-substring (point-min) (point-max)))
	  (cmd  (storm-edit-cmd))
	  (from (1+ (second cmd)))
	  (to   (1+ (third cmd)))
	  (str  (fourth cmd))
	  (errors 0))

     ;; 'execute' the command
     (unless (eq (first cmd) 'edit)
       (error "Invalid command, only 'edit is supported."))

     (let ((inhibit-modification-hooks t))
       ;; We call hooks ourselves to make sure we only notify once!
       (goto-char from)
       (unless (= to from)
	 (delete-char (- to from)))
       (unless (= 0 (length str))
	 (insert str)))

     ;; Call hooks!
     (call-changed-hooks from (point) (- to from))
     (storm-update-colors)
     (redisplay)


     (when (eq bench-batch-run 'error)
       (setq errors (storm-compare-edit ref from to str))
       (with-current-buffer bench-incr-buffer
	 (insert (format "%5d\t%s\n" errors file)))

       (storm-debug-re-open)
       (storm-update-colors)
       (redisplay)
       (setq errors (storm-compare-edit ref from to str))
       (with-current-buffer bench-raw-buffer
	 (insert (format "%5d\t%s\n" errors file)))
       (redisplay)
       )

     (if (eq bench-batch-run 'nil)
	 (let ((errors (storm-compare-edit ref from to str)))
	   (redisplay)
	   (when (> errors 0)
	     (storm-output-string (format "%S errors in %s\n" errors file) 'storm-bench-fail))
	   errors)
       ;; Always close buffers in batch mode.
       0))))

(defun storm-compare-edit (ref from to str)
  (+ (storm-compare-buffer-part ref (point-min) 0 (1- from))
     (storm-compare-buffer-part ref (+ from (length str)) (1- to) (- (length ref) to))))

(defun storm-edit-cmd ()
  (goto-char (point-min))
  (search-forward " " (line-end-position))
  (first (read-from-string (buffer-substring-no-properties (point) (line-end-position)))))

(defun call-changed-hooks (edit-begin edit-end old-length)
  (dolist (hook after-change-functions)
    (unless (eq hook t)
      (funcall hook edit-begin edit-end old-length))))


;; (let ((bench-batch-run 'error))
;;   (storm-run-benchmarks "test/server-tests/apache/incomplete/")
;;   (storm-run-benchmarks "test/server-tests/apache/random/")
;;   (storm-run-benchmarks "test/server-tests/apache/scope/")
;;   (storm-run-benchmarks "test/server-tests/apache/string/")
;;   )
