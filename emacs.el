;; Configuration

(setq project-root "~/Projects/storm/")
(setq project-file (concat project-root "storm.sln"))
(setq add-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl"))
(setq p-compile-command "scripts/compile")
(setq p-clean-command "scripts/compile -c")
(setq p-release-command "scripts/compile -r")
(setq p-all-command "scripts/compile -a")

;; Setup code-style

(require 'whitespace)
(setq whitespace-style '(face trailing lines-tail))
(setq whitespace-line-column 120)

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
	    ;; Enable kernel mode for the appropriate files
	    (when (in-project buffer-file-name)
	      (setq ff-other-file-alist my-cpp-other-file-alist)
	      (setq indent-tabs-mode t)
	      (whitespace-mode t)
	      (c-set-style "linux-tabs-only")
	      (setq tab-width 4)
	      (setq c-basic-offset 4)
	      )))

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
    (indent-for-tab-command)
    (goto-char to)))

(defun storm-cpp-single ()
  (interactive "*")
  (insert "// "))

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

(global-set-key (kbd "C-.") 'other-window)

(add-hook 'c-mode-common-hook
	  (lambda ()
	    (local-set-key (kbd "M-o") 'ff-find-other-file)
	    (local-set-key (kbd "RET") 'storm-return)
	    (local-set-key (kbd "C-M-j") 'storm-cpp-singleline)
	    (local-set-key (kbd "C-M-k") 'storm-insert-comment)
	    (local-set-key (kbd "C->") "->")
	    (local-set-key (kbd "M-p") 'compile-project)
	    (local-set-key (kbd "C-c C-m") 'clean-project)
	    (local-set-key (kbd "C-c C-r") 'compile-release)
	    (local-set-key (kbd "C-c C-a") 'compile-all)
	    (local-set-key (kbd "C-c C-v C-s") 'open-vs)
	    (local-set-key (kbd "M-n") 'next-error)
	    )
	  )

;; Helpers for bindings.

(defun in-project (filename)
  (and filename
       (string-match (expand-file-name project-root) filename)))

(defun subpath (filename)
  (substring (expand-file-name filename) (length (expand-file-name project-root))))

(defun subproject (filename)
  (let ((l (split-string (subpath filename) "/")))
	(car l)))

(defun filename (fname)
  (let ((l (split-string (subpath fname) "/")))
	(nth (- (length l) 1) l)))

(add-hook 'find-file-hooks 'maybe-add-cpp-template)
(defun maybe-add-cpp-template ()
  (interactive)
  (if (and (in-project buffer-file-name)
		   (not (file-exists-p buffer-file-name)))
      (if (string-match "\\.cpp$" buffer-file-name)
		  (add-cpp-template)
		(if (string-match "\\.h$" buffer-file-name)
			(add-header-template)))))

(defun add-cpp-template ()
  (insert "#include \"stdafx.h\"\n")
  (insert "#include \"")
  (insert (replace-regexp-in-string
		   ".cpp" ".h"
		   (filename buffer-file-name)))
  (insert "\"\n\n")
  (insert "namespace ")
  (insert (downcase (subproject buffer-file-name)))
  (insert " {\n\n")
  (let ((pos (point)))
    (insert "\n\n}\n")
    (goto-char pos)
    (indent-for-tab-command))
  (add-file-to-project buffer-file-name))

(defun add-header-template ()
  (insert "#pragma once\n\n")
  (insert "namespace ")
  (insert (downcase (subproject buffer-file-name)))
  (insert " {\n\n")
  (let ((pos (point)))
    (insert "\n\n}\n")
    (goto-char pos)
    (indent-for-tab-command))
  (add-file-to-project buffer-file-name))

(defun add-file-to-project (file)
  (let ((subproj (subproject file)))
    (let ((projfile (concat (expand-file-name project-root) subproj "/" subproj ".vcproj")))
      (let ((cmd (concat add-file-cmd " \"" (expand-file-name projfile) "\" " (substring (subpath file) (+ 1 (length subproj))))))
		(shell-command-to-string cmd)))))

(defun open-vs ()
  (interactive)
  (start-process-shell-command "vs" nil (concat "devenv " (expand-file-name project-file))))

(defun compile-project ()
  (interactive)
  (run-compile p-compile-command))

(defun clean-project ()
  (interactive)
  (run-compile p-clean-command))

(defun compile-release ()
  (interactive)
  (run-compile p-release-command))

(defun compile-all ()
  (interactive)
  (run-compile p-all-command))

(defun run-compile (cmd)
  (let ((default-directory project-root))
    (compile cmd)))

;; Behaviour.

(add-to-list 'auto-mode-alist '("\\.h" . c++-mode))

(setq display-buffer-alist
      '((".*" . ((display-buffer-reuse-window
		  display-buffer-use-some-window
		  display-buffer-pop-up-window)
		 . nil))))

(setq compilation-scroll-output 'first-error)

(defvar my-cpp-other-file-alist
  '(("\\.cpp\\'" (".h"))
    ("\\.h\\'" (".cpp" ".c"))
    ))
