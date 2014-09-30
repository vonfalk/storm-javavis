;; Configuration

(setq project-root "~/Projects/storm/")
(setq project-file (concat project-root "storm.sln"))
(setq add-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -a"))
(setq remove-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -d"))
(setq rename-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -r"))
(setq p-compile-command "scripts/compile")
(setq p-clean-command "scripts/compile -c")
(setq p-release-command "scripts/compile -r")
(setq p-all-command "scripts/compile -a")
(setq read-buffer-completion-ignore-case t)

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

(defun storm-cpp-singleline ()
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
	    (local-set-key (kbd "C-o") 'my-open-line)
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
	    (local-set-key (kbd "C-c C-f C-r") 'rename-proj-file)
	    (local-set-key (kbd "C-c C-f C-d") 'delete-proj-file)
	    )
	  )

;; Helpers for bindings.

(defun my-open-line ()
  (interactive)
  (open-line 1)
  (let ((last (point)))
    (move-beginning-of-line 2)
    (indent-for-tab-command)
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
  (let ((l (split-string (subpath fname) "/")))
    (car (last l))))

(defun subproj-relative-file (filename)
  (let ((subproj (subproject filename)))
    (substring (subpath filename) (+ 1 (length subproj)))))

(defun project-file (file)
  (let ((subproj (subproject file)))
    (concat (expand-file-name project-root) subproj "/" subproj ".vcproj")))

(defun project-cmd-helper (files)
  (if (eq (cdr files) nil)
      (subproj-relative-file (car files))
    (concat (subproj-relative-file (car files))
	    " " (project-cmd-helper (cdr files)))))

(defun project-cmd (cmd files)
  (concat cmd
	  " " (expand-file-name (project-file (car files)))
	  " " (project-cmd-helper files)))

(add-hook 'find-file-hooks 'maybe-add-cpp-template)
(defun maybe-add-cpp-template ()
  (interactive)
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
  (add-file-to-project buffer-file-name))

(defun add-header-template ()
  (insert "#pragma once\n\n")
  (if (shall-have-namespace)
      (insert-namespace))
  (add-file-to-project buffer-file-name))

(defun is-test-project ()
  (let ((proj (subproject buffer-file-name)))
    (and (> (length proj) 4)
	 (eql (substring proj -4 "Test")))))

(defun shall-have-namespace ()
  (let ((proj (subproject buffer-file-name)))
    (or
     (is-test-project)
     (eql (proj "Utils")))))
  
(defun insert-namespace ()
  (insert "namespace ")
  (insert (downcase (subproject buffer-file-name)))
  (insert " {\n\n")
  (let ((pos (point)))
    (insert "\n\n}\n")
    (goto-char pos)
    (indent-for-tab-command)))

(defun add-file-to-project (file)
  (shell-command-to-string (project-cmd add-file-cmd (list file))))

(defun rename-file-project (from to)
  (shell-command-to-string (project-cmd rename-file-cmd (list from to))))

(defun remove-file-project (file)
  (shell-command-to-string (project-cmd remove-file-cmd (list file))))

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

(defun rename-proj-file ()
  (interactive)
  (let ((fn (read-file-name "New name:")))
    (rename-file-project buffer-file-name fn)
    (rename-file buffer-file-name fn)
    (setq buffer-file-name fn)
    (rename-buffer (filename fn) t)))

(defun delete-proj-file ()
  (interactive)
  (if (yes-or-no-p "Really delete?")
      (progn
	(remove-file-project buffer-file-name)
	(delete-file buffer-file-name)
	(kill-buffer))
    ))

;; Behaviour.

(add-to-list 'auto-mode-alist '("\\.h" . c++-mode))

(setq display-buffer-alist
      '(("\\*compilation\\*" .
	 ((display-buffer-reuse-window
	   display-buffer-use-some-window
	   display-buffer-pop-up-window)
	  . ((reusable-frames . t)
	     (inhibit-switch-frame . t))))
	(".*" .
	 ((display-buffer-reuse-window
	   display-buffer-use-some-window
	   display-buffer-pop-up-window) .
	   nil))))
	 


(setq compilation-scroll-output 'first-error)

(defvar my-cpp-other-file-alist
  '(("\\.cpp\\'" (".h"))
    ("\\.h\\'" (".cpp" ".c"))
    ))
