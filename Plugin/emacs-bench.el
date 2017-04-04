;; Benchmark tests for the language server.


;; Run tests using C-c b
(global-set-key (kbd "C-c b") 'storm-run-benchmarks)

;; Test root directory.
(defvar benchmark-root-dir "test/benchmark/" "Benchmarks root directory, relative to 'storm-mode-root'.")

;; Run benchmarks.
(defun storm-run-benchmarks ()
  "Run benchmarks. Make sure Storm is running before starting the benchmarks."
  (interactive)
  (let ((old-storm-mode global-storm-mode)
	(old-chunk-size (storm-query '(chunk-size))))
    (setq global-storm-mode nil)
    (storm-send '(chunk-size 0))
    (storm-run-dir (concat (file-name-as-directory storm-mode-root) benchmark-root-dir))
    (setq global-storm-mode old-storm-mode)
    (storm-send (cons 'chunk-size old-chunk-size))))

(defun storm-run-dir (dir)
  "Run benchmarks in a directory."
  (setq dir (file-name-as-directory dir))
  (let ((files (directory-files-and-attributes dir)))
    (while (consp files)
      (storm-run-file-or-dir
       dir
       (car (car files))
       (cdr (car files)))
      (setq files (cdr files)))))

(defun storm-run-file-or-dir (path file attrs)
  "Run the test specified in the file."
  (cond ((= (string-to-char file) ?.)
	 nil)
	((= (string-to-char file) ?#)
	 nil)
	((eq (first attrs) 't)
	 (storm-run-dir (concat path file)))
	(t (storm-run-file path file))))

(defun storm-run-file (path file)
  "Run the specified file (if supported)."
  (let ((dispatch-to nil))
    (when (storm-supports (file-name-extension file))
      (setq dispatch-to (storm-find-type file)))

    (if dispatch-to
	(progn
	  (storm-output-string (format "Running file: %s%s...\n" path file) 'storm-bench-msg)
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

(defun storm-open-buffer (file)
  "Open a buffer, wait until it is fully colored and ready for use."
  (save-excursion
    (let ((buf (find-file-noselect file)))
      (display-buffer buf t)
      (with-current-buffer buf
	(storm-mode)
	(storm-wait-for 'color 5.0)
	buf))))

(defface storm-bench-msg
  '((t :foreground "dark green"))
  "Face used indicating test status.")

(defface storm-bench-fail
  '((t :foreground "red"))
  "Face used indicating test failures.")

(defface storm-hilight-diff-face
  '((t :background "red"))
  "Face used to indicate erroneous syntax highlighting.")

(defun storm-compare-buffer (ref-str)
  (let ((pos 0)
	(to (- (point-max) (point-min)))
	(errors 0))
    (while (< pos to)
      (let* ((buf-pos (+ (point-min) pos))
	     (cur (get-text-property buf-pos 'font-lock-face))
	     (ref (get-text-property pos 'font-lock-face ref-str)))
	(unless (equal cur ref)
	  (with-silent-modifications
	    (put-text-property buf-pos (1+ buf-pos) 'font-lock-face 'storm-hilight-diff-face))
	  (setq errors (1+ errors))))
      (setq pos (1+ pos)))
    errors))


(defvar storm-bench-types
  '(
    ("fill" . storm-run-fill)
    )
  "Kinds of tests that are currently supported.")

(defun storm-run-fill (file)
  "Run files with .fill.X."
  (let ((buffer (storm-open-buffer file)))
    (with-current-buffer buffer
      (let ((ref (buffer-substring (point-min) (point-max))))
	(erase-buffer)
	(storm-wait-for 'color 1.0)
	;; Re-open the current buffer, so we start with a clean slate!
	(storm-debug-re-open)

	(while (<= (point-max) (length ref))
	  (insert (substring-no-properties ref (1- (point-max)) (point-max)))
	  (storm-wait-for 'color 1.0)
	  (redisplay))

	(set-buffer-modified-p nil)

	(let ((result (storm-compare-buffer ref)))
	  (when (> result 0)
	    (storm-output-string (format "%S errors in %s\n" result file) 'storm-bench-fail)))))))
