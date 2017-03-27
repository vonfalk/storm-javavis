;; Configuration

(modify-coding-system-alist 'file "\\.bs\\'" 'utf-8-dos)
(add-to-list 'auto-mode-alist '("\\.h" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.bs" . java-mode))

(defvar my-cpp-other-file-alist
  '(("\\.cpp\\'" (".h" ".hpp"))
    ("\\.h\\'" (".cpp" ".c"))
    ("\\.c\\'" (".h"))
    ))

(add-hook 'compilation-mode-hook
	  (lambda ()
	    (setq tab-width 4)))

(setq project-root "~/Projects/storm/")


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
      (storm-open-line)
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

(global-set-key (kbd "M-g c") 'goto-char)
(global-set-key (kbd "M-g M-c") 'goto-char)


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
  (unless (insert-file-template-p)
    (insert "#include \"stdafx.h\"\n")
    (unless (is-test-project)
      (insert "#include \"")
      (insert (replace-regexp-in-string
	       "\\.cpp" ".h"
	       (filename buffer-file-name)))
      (insert "\""))
    (insert "\n\n")
    (if (shall-have-namespace)
	(insert-namespace))
    (if (is-test-project)
	(insert-test-template))))

(defun add-header-template ()
  (unless (insert-file-template-p)
    (insert "#pragma once\n\n")
    (if (shall-have-namespace)
	(insert-namespace))))

(defun insert-file-template-p ()
  "Inserts a template from the file 'ext'.template in the same directory as the file"
  (let* ((dir (file-name-directory buffer-file-name))
	 (ext (file-name-extension buffer-file-name))
	 (tName (concat dir ext ".template")))
    (if (file-exists-p (concat dir ext ".template"))
	(progn
	  (insert-file-template tName)
	  t)
      nil)))

(defun insert-file-template (file)
  (insert-file-contents file)

  ;; Replace $header$...
  (goto-char 0)
  (perform-replace "$header$"
		   (replace-regexp-in-string "\\.cpp" ".h" (filename buffer-file-name))
		   nil
		   nil
		   nil)


  ;; Replace $file$...
  (goto-char 0)
  (perform-replace "$file$"
		   (replace-regexp-in-string "\\.cpp" "" (filename buffer-file-name))
		   nil
		   nil
		   nil)

  ;; Find $$ and put the cursor there.
  (goto-char 0)
  (if (re-search-forward "\\$\\$" nil t)
      (progn
	(replace-match "")
	(indent-for-tab-command))))

(defun insert-test-template ()
  (insert "BEGIN_TEST(")
  (insert (replace-regexp-in-string "\\.cpp" "" (filename buffer-file-name)))
  (insert ") {\n")
  (indent-for-tab-command)
  (insert "Engine &e = gEngine();\n\n")
  (let ((pos (point)))
    (insert "\n\n} END_TEST")
    (indent-for-tab-command)
    (goto-char pos)
    (indent-for-tab-command)))

(defun is-test-project ()
  (let ((proj (subproject buffer-file-name)))
    (and (>= (length proj) 4)
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

;; (defun check-buffer ()
;;   "If the buffer has been modified, ask the user to revert it, just like find-file does."
;;   (interactive)
;;   (if (and (not (verify-visited-file-modtime))
;; 	   (not (buffer-modified-p))
;; 	   (yes-or-no-p
;; 	    (format "File %s changed on disk. Reload from disk? " (file-name-nondirectory (buffer-file-name)))))
;;       (revert-buffer t t)))

;; (defadvice switch-to-buffer
;;   (after check-buffer-modified)
;;   (check-buffer))

;;(ad-activate 'switch-to-buffer)

;; Good stuff while using the plugin.
(load "~/Projects/storm/Plugin/emacs.el")
(load "~/Projects/storm/Plugin/emacs-test.el")
(setq storm-mode-root "~/Projects/storm/root")
(setq storm-mode-compiler "~/Projects/storm/debug/Storm.exe")
(setq storm-mode-compile-compiler t)


;;; For debugging ;;;

(global-set-key (kbd "C-M-u") 'storm-restart)
(global-set-key (kbd "C-M-y") 'storm-stop)
(global-set-key (kbd "C-c s") 'tmp-send)
(defun tmp-send ()
  (interactive)
  (storm-send '(22 10 ("hej" storm) storm)))

;; For debugging mymake when needed...
;;(setq mymake-command "C:/Users/Filip/Projects/mymake/release/mymake.exe")
