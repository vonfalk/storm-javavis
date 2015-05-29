;; Configuration

(setq font-lock-maximum-decoration 2)

(setq use-compilation-window t)
(setq project-root "~/Projects/storm/")
(setq project-file (concat project-root "storm.sln"))
(setq add-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -a"))
(setq remove-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -d"))
(setq rename-file-cmd (concat "perl " (expand-file-name project-root) "scripts/add.pl -r"))
(setq p-compile-command "scripts/compile")
(setq p-compile-valgrind-command "scripts/compile -v")
(setq p-clean-command "scripts/compile -c")
(setq p-release-command "scripts/compile -r")
(setq p-all-command "scripts/compile -a")
(setq read-buffer-completion-ignore-case t)

(setq compilation-w 100)
(setq compilation-h 83)
(setq compilation-adjust 160)

;; Demo mode

(setq demo-mode nil)

(defun start-demo (name)
  (setq demo-mode t)
  (set-face-attribute 'default nil :height 100))

(add-to-list 'command-switch-alist '("demo" . start-demo))

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


;; Setup code-style

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
	    ;; Enable kernel mode for the appropriate files
	    (when t ;; (in-project buffer-file-name)
	      (setq tab-width 4)
	      (setq ff-other-file-alist my-cpp-other-file-alist)
	      (setq ff-special-constructs nil)
	      (setq indent-tabs-mode t)
	      (c-set-style "linux-tabs-only")
	      (whitespace-mode t)
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

(global-set-key (kbd "C-.") 'other-window)
(global-set-key (kbd "M-g c") 'goto-byte)
(global-set-key (kbd "M-g M-c") 'goto-byte)

(global-set-key (kbd "M-p") 'compile-project)
(global-set-key (kbd "C-c M-p") 'compile-project-valgrind)
(global-set-key (kbd "C-c C-m") 'clean-project)
(global-set-key (kbd "C-c C-r") 'compile-release)
(global-set-key (kbd "C-M-p") 'compile-all)
(global-set-key (kbd "C-c C-k") 'kill-compilation)
(global-set-key (kbd "C-c C-v C-s") 'open-vs)

(add-hook 'c-mode-common-hook
	  (lambda ()
	    (local-set-key (kbd "C-o") 'my-open-line)
	    (local-set-key (kbd "M-o") 'ff-find-other-file)
	    (local-set-key (kbd "RET") 'storm-return)
	    (local-set-key (kbd "C-M-j") 'storm-cpp-singleline)
	    (local-set-key (kbd "C-M-k") 'storm-insert-comment)
	    (local-set-key (kbd "C->") "->")
	    (local-set-key (kbd "M-n") 'next-error)
	    (local-set-key (kbd "C-c C-f C-r") 'rename-proj-file)
	    (local-set-key (kbd "C-c C-f C-d") 'delete-proj-file)
	    )
	  )

;; Helpers for bindings.

(defun my-open-line ()
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
      (insert-test-template))
  (add-file-to-project buffer-file-name))

(defun add-header-template ()
  (insert "#pragma once\n\n")
  (if (shall-have-namespace)
      (insert-namespace))
  (add-file-to-project buffer-file-name))

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
      (equal proj "StormBuiltin")))))

(defun namespace-name ()
  (let ((name (downcase (subproject buffer-file-name))))
    (if (string-equal name "lib")
	"storm"
      name)))

(defun insert-namespace ()
  (insert "namespace ")
  (insert (namespace-name))
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
  (if (in-project buffer-file-name)
      (run-compile p-compile-command)
    (call-interactively 'compile)))

(defun compile-project-valgrind ()
  (interactive)
  (run-compile p-compile-valgrind-command))

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
    (compile cmd t)))

(defun rename-proj-file ()
  (interactive)
  (let ((fn (read-file-name "New name:")))
    (rename-file-project buffer-file-name fn)
    (rename-file buffer-file-name fn)
    (set-visited-file-name fn t t)))

(defun delete-proj-file ()
  (interactive)
  (if (yes-or-no-p "Really delete?")
      (progn
	(remove-file-project buffer-file-name)
	(delete-file buffer-file-name)
	(kill-buffer))
    ))

;; Create compilation window
(setq compilation-window 'nil)
(setq compilation-frame 'nil)

(defun create-compilation-frame ()
  (let* ((current-params (frame-parameters))
	 (left-pos (cdr (assoc 'left current-params)))
	 (height (cdr (assoc 'height current-params)))
	 (created (make-frame '(inhibit-switch-frame))))
    (setq compilation-frame created)
    (setq compilation-window (frame-selected-window created))
    (set-frame-size created compilation-w compilation-h)
    (let* ((created-w (frame-pixel-width created)))
      (set-frame-position created (- left-pos created-w compilation-adjust) 0)
      )))

(defun get-compilation-window (buffer e)
  (if demo-mode
      nil
    (progn
      (if (eq compilation-frame 'nil)
	  (create-compilation-frame)
	(if (not (frame-live-p compilation-frame))
	    (create-compilation-frame)))
      (window--display-buffer buffer compilation-window 'frame)
      compilation-window)))


;; Behaviour.

(add-to-list 'auto-mode-alist '("\\.h" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.bs" . java-mode))

(setq display-buffer-alist
      '(("\\*compilation\\*" .
	 ((display-buffer-reuse-window
	   get-compilation-window
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

(defun check-buffer ()
  "If the buffer has been modified, ask the user to revert it, just like find-file does."
  (interactive)
  (if (and (not (verify-visited-file-modtime))
	   (not (buffer-modified-p))
	   (yes-or-no-p
	    (format "File %s changed on disk. Reread from disk? " (file-name-nondirectory (buffer-file-name)))))
      (revert-buffer t t)))

(defadvice switch-to-buffer
  (after check-buffer-modified)
  (check-buffer))

(ad-activate 'switch-to-buffer)

