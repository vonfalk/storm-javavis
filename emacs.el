;; Configuration

(modify-coding-system-alist 'file "\\.bs\\'" 'utf-8-dos)
(add-to-list 'auto-mode-alist '("\\.h" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.bs" . java-mode))

(defvar my-cpp-other-file-alist
  '(("\\.cpp\\'" (".h" ".hpp"))
    ("\\.h\\'" (".cpp" ".c"))
    ("\\.c\\'" (".h"))
    ))

(setq project-root "~/Projects/storm/")

;; Mark errors in compilation-mode. TODO: Manage to convert offsets to line+col as well!
(require 'compile)
(pushnew '(storm "^ *\\([0-9]+>\\)?\\([^(\n]+\\)(\\([0-9]+\\),\\([0-9]+\\)):" 2 3 4 nil)
	 compilation-error-regexp-alist-alist)
(add-to-list 'compilation-error-regexp-alist 'storm)


;; Custom goto-char (including line endings)

(defun newline-on-disk ()
  (let* ((sym buffer-file-coding-system)
	 (name (symbol-name sym))
	 (end (substring name -4)))
    (if (equal end "-dos")
	2
      1)))

(defun goto-byte (byte)
  (interactive "nGoto byte: ")
  (setq pos 0)
  (setq lines 0)
  (setq last-line-nr (line-number-at-pos pos))
  (setq line-weight (- (newline-on-disk) 1))
  (while (<= (+ pos lines) byte)
    (setq line-nr (line-number-at-pos pos))
    (setq pos (+ pos 1))
    (if (not (= last-line-nr line-nr))
	(setq lines (+ lines line-weight)))
    (setq last-line-nr line-nr))
  (goto-char pos))


;; Setup code-style. From the Linux Kernel Coding style.

(require 'whitespace)
(setq whitespace-style '(face trailing lines-tail))
(setq whitespace-line-column 120)

(defun blank-line ()
  (= (point) (line-end-position)))

(defun c-lineup-arglist-tabs-only (ignored)
  "Line up argument lists by tabs, not spaces"
  (let* ((anchor (c-langelem-pos c-syntactic-element))
	 (column (c-langelem-2nd-pos c-syntactic-element))
	 (offset (- (1+ column) anchor))
	 (steps (floor offset c-basic-offset)))
    (* (max steps 1)
       c-basic-offset)))

(add-hook 'c-mode-common-hook
          (lambda ()
            ;; Add kernel style
            (c-add-style
             "linux-tabs-only"
             '("linux" (c-offsets-alist
                        (arglist-cont-nonempty
                         c-lineup-gcc-asm-reg
                         c-lineup-arglist-tabs-only))))
	    (setq tab-width 4)
	    (setq ff-other-file-alist my-cpp-other-file-alist)
	    (setq ff-special-constructs nil)
	    (setq indent-tabs-mode t)
	    (c-set-style "linux-tabs-only")
	    (whitespace-mode t)
	    (setq c-basic-offset 4)

	    ;; Better handling of comment-indentation. Why needed?
	    (c-set-offset 'comment-intro 0)))

(defun storm-insert-comment ()
  "Insert a comment in c-mode"
  (interactive "*")
  (insert "/**")
  (indent-for-tab-command)
  (insert "\n * ")
  (indent-for-tab-command)
  (let ((to (point)))
    (insert "\n */")
    (indent-for-tab-command)
    (insert "\n")
    (if (not (blank-line))
	(indent-for-tab-command))
    (goto-char to)))

(defun storm-cpp-singleline ()
  (interactive "*")
  (if (blank-line)
      (insert "// ")
    (progn
      (my-open-line)
      (insert "// "))))

(defun storm-return () 
  "Advanced `newline' command for Javadoc multiline comments.   
   Insert a `*' at the beggining of the new line if inside of a comment."
  (interactive "*")
  (let* ((last (point))
         (is-inside
          (if (search-backward "*/" nil t)
              ;; there are some comment endings - search forward
              (search-forward "/*" last t)
            ;; it's the only comment - search backward
            (goto-char last)
            (search-backward "/*" nil t))))

    ;; go to last char position
    (goto-char last)

    ;; the point is inside some comment, insert `* '
    (if is-inside
        (progn
          (newline-and-indent)
          (insert "*")
	  (insert " ")
	  (indent-for-tab-command))
      ;; else insert only new-line
      (newline-and-indent))))

;; Setup keybindings

(global-set-key (kbd "M-g c") 'goto-byte)
(global-set-key (kbd "M-g M-c") 'goto-byte)


(add-hook 'c-mode-common-hook
	  (lambda ()
	    (local-set-key (kbd "M-o") 'ff-find-other-file)
	    (local-set-key (kbd "C-o") 'storm-open-line)
	    (local-set-key (kbd "RET") 'storm-return)
	    (local-set-key (kbd "C-M-j") 'storm-cpp-singleline)
	    (local-set-key (kbd "C-M-k") 'storm-insert-comment)
	    (local-set-key (kbd "C->") "->")
	    )
	  )

;; Helpers for bindings.

(defun storm-open-line ()
  (interactive "*")
  (open-line 1)
  (let ((last (point)))
    (move-beginning-of-line 2)
    (if (not (blank-line))
	(indent-for-tab-command))
    (goto-char last)))

(defun in-project (filename)
  (and filename
       (string-match (expand-file-name project-root) filename)))

(defun subpath (filename)
  (substring (expand-file-name filename) (length (expand-file-name project-root))))

(defun subproject (filename)
  (let ((l (split-string (subpath filename) "/")))
    (car l)))

(defun filename (fname)
  (let ((l (split-string fname "/")))
    (car (last l))))

(defun subproj-relative-file (filename)
  (let ((subproj (subproject filename)))
    (substring (subpath filename) (+ 1 (length subproj)))))

(add-hook 'find-file-hooks 'maybe-add-cpp-template)
(defun maybe-add-cpp-template ()
  (interactive "*")
  (if (and (in-project buffer-file-name)
	   (not (file-exists-p buffer-file-name)))
      (if (string-match "\\.cpp$" buffer-file-name)
	  (add-cpp-template)
	(if (string-match "\\.h$" buffer-file-name)
	    (add-header-template)))))

(add-hook 'find-file-hooks 'correct-win-filename)
(defun correct-win-filename ()
  (interactive)
  (rename-buffer (filename buffer-file-name) t))

(defun add-cpp-template ()
  (insert "#include \"stdafx.h\"\n")
  (insert "#include \"")
  (if (is-test-project)
      (insert "Test/Test.h")
    (insert (replace-regexp-in-string
	     ".cpp" ".h"
	     (filename buffer-file-name))))
  (insert "\"\n\n")
  (if (shall-have-namespace)
      (insert-namespace))
  (if (is-test-project)
      (insert-test-template)))

(defun add-header-template ()
  (insert "#pragma once\n\n")
  (if (shall-have-namespace)
      (insert-namespace)))

(defun insert-test-template ()
  (insert "BEGIN_TEST(")
  (insert (replace-regexp-in-string ".cpp" "" (filename buffer-file-name)))
  (insert ") {\n\n")
  (let ((pos (point)))
    (insert "\n\n} END_TEST")
    (goto-char pos)
    (indent-for-tab-command)))

(defun is-test-project ()
  (let ((proj (subproject buffer-file-name)))
    (and (> (length proj) 4)
	 (equal (substring proj -4) "Test"))))

(defun shall-have-namespace ()
  (let ((proj (subproject buffer-file-name)))
    (not
     (or
      (is-test-project)
      (equal proj "Utils")
      (equal proj "CppTypes")
      (equal proj "StormBuiltin")))))

(defun namespace-name ()
  (let ((name (downcase (subproject buffer-file-name))))
    (cond ((string-equal name "shared") "storm")
	  ((string-equal name "runtime") "storm")
	  ((string-equal name "compiler") "storm")
	  ((string-equal name "core") "storm")
	  (t name))))

(defun insert-namespace ()
  (insert "namespace ")
  (insert (namespace-name))
  (insert " {\n\n")
  (let ((pos (point)))
    (insert "\n\n}\n")
    (goto-char pos)
    (indent-for-tab-command)))

;; Behaviour.

(setq compilation-scroll-output 'first-error)

(defun check-buffer ()
  "If the buffer has been modified, ask the user to revert it, just like find-file does."
  (interactive)
  (if (and (not (verify-visited-file-modtime))
	   (not (buffer-modified-p))
	   (yes-or-no-p
	    (format "File %s changed on disk. Reload from disk? " (file-name-nondirectory (buffer-file-name)))))
      (revert-buffer t t)))

(defadvice switch-to-buffer
  (after check-buffer-modified)
  (check-buffer))

(ad-activate 'switch-to-buffer)

