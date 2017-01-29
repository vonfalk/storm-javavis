;; Implementation of the Storm mode for Emacs.
;;
;; This mode is a bit special compared to other modes. Once enabled, it spawns a Storm process. This
;; process is then used to provide syntax highlighting for all supported file types, even if
;; storm-mode is not configured to use those file types.
;;
;; Use 'global-storm-mode' to start storm-mode globally, or 'storm-mode' to use storm-mode for a
;; single buffer.

;; Configuration.
(defvar storm-mode-root nil "Root of the Storm source tree.")
(defvar storm-mode-compiler nil "Path to the Storm compiler.")
(defvar storm-mode-stdlib nil "Root of the Storm standard library (currently not used).")
(defvar storm-mode-compile-compiler nil "Try to compile the compiler before starting it.")
(defvar storm-mode-max-edits 50 "Maximum number of entries saved in the edit history.")

;; State for all buffers using storm-mode.
(defvar storm-mode-buffers (make-hash-table) "Map int->buffer for all buffers and their associated id.")
(defvar storm-mode-next-id 0   "Next usable buffer ID in storm-mode.")
(defvar storm-started-hook nil "Hook executed when the storm process has started.")
(defvar-local storm-buffer-id nil "The ID of the current buffer in storm-mode.")
(defvar-local storm-buffer-edit-id 0 "The ID of the next edit operation for the current buffer.")
(defvar-local storm-buffer-edits nil
  "List of the last few edits to this buffer. This is so we can handle 'late' coloring messages from Storm.")
(defvar-local storm-buffer-no-changes nil "Inhibit change notifications for the current buffer.")

(defun global-storm-mode ()
  "Use storm-mode for all applicable buffers."
  (interactive)
  (storm-start)
  ;; TODO: Look at all open buffers and use storm-mode for them.
  ;; TODO: Create a hook so that we get notified about all newly opened files.
  )

(defun storm-mode ()
  "Use storm-mode for the current buffer."
  (interactive)
  (kill-all-local-variables)
  ;; For now, we use the default syntax table.
  (use-local-map storm-mode-map)
  (set (make-local-variable 'indent-line-function) 'storm-indent-line)
  (setq major-mode 'storm-mode)
  (setq mode-name "Storm")
  (storm-register-buffer (current-buffer))
  (add-hook 'kill-buffer-hook 'storm-buffer-killed)
  (add-hook 'change-major-mode-hook 'storm-buffer-killed)
  (add-hook 'after-change-functions 'storm-buffer-changed)

  (run-mode-hooks 'storm-mode-hook))

(provide 'storm-mode)

(defun storm-indent-line ()
  "Indent the current line in storm-mode."
  (interactive)
  )

(defvar storm-mode-hook nil "Hook run when storm-mode is initialized.")

(defvar storm-mode-map
  (let ((map (make-keymap)))
    ;(define-key map "C-M-u" 'storm-update-buffer)
    map)
  "Keymap for storm-mode")

;; Convenience for highlighting.

(defun storm-set-color (from to face)
  "Convenience for highlighting parts of the text."
  (with-silent-modifications
    (put-text-property from to 'font-lock-face face)))


(defvar storm-colors
  (let ((map (make-hash-table)))
    (puthash 'comment 'font-lock-comment-face map)
    (puthash 'delimiter 'font-lock-delimiter-face map)
    (puthash 'string 'font-lock-string-face map)
    (puthash 'constant 'font-lock-constant-face map)
    (puthash 'keyword 'font-lock-keyword-face map)
    (puthash 'fn-name 'font-lock-function-name-face map)
    (puthash 'var-name 'font-lock-variable-name-face map)
    (puthash 'type-name 'font-lock-type-name-face map)
    map)
  "Lookup color names in emacs lisp")

(defun storm-find-color (color)
  "Find the proper symbol representing 'color'."
  (gethash color storm-colors nil))

(defface storm-server-killed
  '((t :foreground "dark red"))
  "Face used indicating the server died for some reason.")

(defface storm-msg-error
  '((t :foreground "red"))
  "Face used indicating communication errors.")

;; Buffer management.

(defun storm-register-buffer (buffer)
  "Register a new buffer with the running storm process."
  (with-current-buffer buffer
    (unless storm-buffer-id
      ;; Not in the list of buffers, add it!
      (setq storm-buffer-id storm-mode-next-id)
      (puthash storm-mode-next-id buffer storm-mode-buffers)
      (setq storm-mode-next-id (1+ storm-buffer-id)))

    ;; Clear the edit history.
    (setq storm-buffer-edit-id 0)
    (setq storm-buffer-edits nil)

    ;; Tell Storm what is happening.
    (storm-send (list 'open
		      storm-buffer-id
		      (buffer-file-name)
		      (buffer-substring-no-properties (point-min) (point-max))))))

(defun storm-buffer-killed ()
  "Called when a buffer is going to be killed."
  (when storm-buffer-id
    (let ((buf-id storm-buffer-id))
      ;; Remove the id early in case we accidentally enter a recursive call...
      (setq storm-buffer-id nil)
      (remhash buf-id storm-mode-buffers)
      (when (storm-running-p)
	(storm-send (list 'close buf-id))))))


(defun storm-buffer-changed (edit-begin edit-end old-length)
  "Called when the contents of a buffer has been changed.
  NOTE: This is called when we set text color of the buffer as well... Inhibit that!"
  (when (and storm-buffer-id (not storm-buffer-no-changes))
    ;; Remember the edit.
    (setq storm-buffer-edit-id (1+ storm-buffer-edit-id))
    (setq storm-buffer-edits
    	  (limit-length
    	   storm-mode-max-edits
    	   (cons (list edit-begin
    		       (+ edit-begin old-length)
    		       (- edit-end edit-begin))
    		 storm-buffer-edits)))


    ;; Tell Storm.
    (storm-send (list 'edit
    		      storm-buffer-id
    		      storm-buffer-edit-id
    		      (1- edit-begin)
    		      (+ edit-begin old-length -1)
    		      (buffer-substring-no-properties edit-begin edit-end)))))

(defun limit-length (max-length list)
  "Limit the length of the list 'list'"
  (let ((at list)
	(pos 1))
    (while (consp at)
      (when (= pos max-length)
	(setcdr at nil))
      (setq at (cdr at))
      (setq pos (1+ pos)))
    list))


;; Handle messages

(defun storm-on-color (params)
  "Handle the color-message."
  (if (>= (length params) 3)
      (let* ((buffer-id (nth 0 params))
	     (edit-id   (nth 1 params))
	     (start-pos (1+ (nth 2 params)))
	     (at        (nthcdr 3 params))
	     (buffer (gethash buffer-id storm-mode-buffers nil)))
	(when buffer
	  (with-current-buffer buffer
	    ;; Make sure we do not act on our own color notifications!
	    (let ((storm-buffer-no-changes t)
		  (highest (point-max)))
	      (while (consp at)
		(storm-set-color
		 start-pos
		 (min highest (+ start-pos (first at)))
		 (storm-find-color (second at)))
		(setq start-pos (+ start-pos (first at)))
		(setq at (nthcdr 2 at)))))))
    "Too few parameters."))


;; Communication with the Storm process.

(defvar storm-process nil "The currently running Storm process.")
(defvar storm-process-output nil "The buffer currently used for output from Storm.")
(defvar storm-process-buffer "" "Buffer used when reading input from the Storm process.")
(defvar storm-process-sym-to-id nil "List of symbols shared with the Storm process.")
(defvar storm-process-id-to-sym nil "List of symbols shared with the Storm process.")
(defvar storm-process-next-id 0 "Next symbol id.")
(defvar storm-process-stop-timer nil
  "Timer used for timeout when killing the storm process along with the function to run afterwards.")
(defvar storm-process-message-queue nil
  "Messages queued for when the storm process was started.")
(defvar storm-messages
  (let ((map (make-hash-table)))
    (puthash 'color 'storm-on-color map)
    map)
  "Messages handled by this plugin.")

(defun storm-start ()
  "Make sure that the background Storm process is up and running. Start it if neccessary."
  (interactive)
  (unless (storm-running-p)
    ;; TODO: Ask for missing parameters!
    (if storm-mode-compile-compiler
	(storm-start-compile)
      (storm-start-compiler))))

(defun storm-stop (&optional when-done)
  "Stop the current Storm process (if any)."
  (interactive)
  (if storm-process
      (progn
	;; Give Storm 2 seconds to terminate gracefully...
	(setq storm-process-stop-timer
	      (cons
	       (run-at-time 2 nil 'storm-kill)
	       when-done))
	;; Send a quit message.
	(storm-output-string "\n\nTerminating...\n" 'storm-server-killed)
	(storm-send '(quit)))
    (when when-done
      (funcall when-done))))

(defun storm-kill ()
  "Kill the storm process."
  (storm-output-string "\n\nKilling storm process...\n" 'storm-server-killed)
  (storm-terminated))

(defun storm-terminated ()
  "Called when the storm process has been terminated (or should be forcefully terminated)."
  (delete-process storm-process)
  (setq storm-process-output nil)
  (setq storm-process nil)
  (when storm-process-stop-timer
    (let ((to-call (cdr storm-process-stop-timer)))
      (cancel-timer (car storm-process-stop-timer))
      (setq storm-process-stop-timer nil)
      (when to-call
	(funcall to-call)))))


(defun storm-restart ()
  "Restart the current Storm process."
  (interactive)
  (storm-stop 'storm-start))

(defun storm-send (message &rest force)
  "Send a message to the Storm process (launching it if it is not running)."
  (if (storm-running-p)
      ;; Keep message ordering if we try to send messages during startup.
      (if (and (endp force) (consp storm-process-message-queue))
	  (setq storm-process-message-queue
		(cons message storm-process-message-queue))
	(send-string storm-process (storm-encode message)))
    (progn
      (setq storm-process-message-queue
	    (cons message storm-process-message-queue))
      (storm-start))))

(defun storm-running-p ()
  "Get the current status of the Storm process."
  (if (endp storm-process)
      nil
    (process-live-p storm-process)))

(defun storm-parent-directory (dir)
  (unless (equal "/" dir)
    (file-name-directory (directory-file-name dir))))

(defvar storm-running-compile nil "The buffer currently used for compiling Storm.")

(require 'compile)
(defun storm-start-compile ()
  "Start the Storm compiler by first compiling it."
  (add-to-list 'compilation-finish-functions 'storm-compilation-finished)
  ;; Make sure not to run multiple compilations.
  (unless storm-running-compile
    (let ((default-directory (storm-parent-directory (file-name-directory storm-mode-compiler)))
	  (compilation-ask-about-save nil))
      (setq storm-running-compile
	    (compile "mm Main -ne")))))

(defun storm-compilation-finished (buffer exit-status)
  "Called whenever a compilation buffer has finished."
  (when (eq buffer storm-running-compile)
    (setq storm-running-compile nil)
    (if (string-prefix-p "finished" exit-status)
	(progn
	  (message "Compilation successful, starting Storm!")
	  (storm-start-compiler))
      (progn
	(message "Compilation failed.")
	(setq storm-process-message-queue nil)))))

(defun storm-start-compiler ()
  "Start the Storm compiler."
  (setq storm-process
	(start-process
	 "*storm-interactive*"
	 nil
	 storm-mode-compiler "--server"))
  (set-process-coding-system storm-process 'binary 'binary)
  (set-process-filter storm-process 'storm-on-message)
  (set-process-sentinel storm-process 'storm-on-status)
  (set-process-query-on-exit-flag storm-process nil)

  ;; Set up the compilation buffer.
  (setq storm-process-output (get-buffer-create "*compilation*"))
  (setq storm-process-buffer "")
  (setq storm-process-sym-to-id (make-hash-table))
  (setq storm-process-id-to-sym (make-hash-table))
  (setq storm-process-next-id 0)
  (with-current-buffer storm-process-output
    (buffer-disable-undo)
    (erase-buffer)
    (setq buffer-read-only t)
    (set-buffer-multibyte t)
    (compilation-mode))

  ;; Re-create any open buffers inside the Storm process.
  (maphash
   (lambda (id buffer)
     (storm-register-buffer buffer))
   storm-mode-buffers)

  ;; Send any queued messages.
  (storm-send-messages storm-process-message-queue)
  (setq storm-process-message-queue nil)

  (run-hooks 'storm-started-hook))


(defun storm-send-messages (queue)
  "Send all messages in the queue, in reverse order (as that is how they are added)."
  (when queue
    (storm-send-messages (cdr queue))
    (storm-send (car queue) t)))

(defun storm-on-message (process input)
  "Receives input from a Storm process and acts accordingly."
  (setq storm-process-buffer
	(storm-decode-message-string (concat storm-process-buffer input))))

(defun storm-decode-message-string (string)
  "Decodes 'input', returns anything that was not processed."
  (let* ((r (storm-decode-messages string))
	 (complete (car r))
	 (remaining (cdr r)))
    (when complete
      ;; Try to output as much as possible.
      (let* ((decoded (decode-coding-string remaining 'utf-8))
	     (remaining-at (1- (length remaining)))
	     (decoded-at (1- (length decoded))))

	;; Make sure we do not have any partial codepoints in the string.
	(while (and (>= decoded-at 0)
		    (>= (aref decoded decoded-at) #x110000))
	  (setq remaining-at (1- remaining-at))
	  (setq decoded-at (1- decoded-at)))

	(storm-output-string (substring decoded 0 (1+ decoded-at)) nil)
	(setq remaining (substring remaining (1+ remaining-at)))))

    remaining))


(defun storm-decode-messages (string)
  "Decode all messages in 'string'. Returns (t remaining) if we managed to parse something,
   or (nil remaining) if we failed parsing things."
  (let ((null-char (position #x00 string))
	(failed 'nil))
    (while (and (not failed)
		(numberp null-char))
      ;; Output the string found in the beginning.
      (storm-output-string (substring string 0 null-char) 'nil)
      (setq string (substring string null-char))

      ;; Try to parse the remaining string.
      (let ((result (storm-decode string)))
	(if (endp result)
	    (setq failed 't)
	  (let ((consumed (car result))
		(message (cdr result)))
	    (setq string (substring string consumed))
	    (storm-handle-message message)
	    (setq null-char (position #x00 string))))))

    ;; Figure out the result.
    (if failed
	(cons nil string)
      (cons t string))))

(defun storm-handle-message (msg)
  "Handle a message"
  (let ((err (storm-handle-message-i msg)))
    (when (stringp err)
      (storm-output-string
       (format "\nError when processing message: %s\n  message: %S\n" err msg) 'storm-msg-error))))

(defun storm-handle-message-i (msg)
  "Handle a message"
  (if (consp msg)
      (let ((found (gethash (car msg) storm-messages)))
	(if found
	    (funcall found (cdr msg))
	  (format "The header %S is not supported." (car msg))))
    "Invalid message format. Expected a list."))


(defun storm-output-string (text face)
  (when storm-process-output
    (with-current-buffer storm-process-output
      (let* ((buffer-read-only nil)
	     (old-point (point))
	     (start (point-max)))
	(goto-char (point-max))
	(insert text)
	(when face
	  (storm-set-color start (point) face))
	(goto-char old-point)))))

(defun storm-on-status (process change)
  "Receives status notifications from the Storm process."
  (when (eq process storm-process)
    (unless (process-live-p storm-process)
      (message "Storm process terminated.")
      (storm-output-string "\n\nStorm process terminated.\n" 'storm-server-killed)
      (storm-terminated))))

;; Decoding messages in a raw string.

(defun storm-state (src)
  "Create a state for decoding messages. car is the current position, cdr is the length."
  (cons 1 (length src)))

(defun storm-state-error (state)
  "Decide if the state is in an error condition. (ie. pos > length)"
  (> (car state) (cdr state)))

(defun storm-state-set-error (state)
  "Make 'state' into an error state."
  (setcar state (1+ (cdr state)))
  'nil)

(defun storm-state-more-p (state more)
  "Decide if there is at least 'more' additional characters."
  (if (endp more)
      nil
    (<= (+ (car state) more) (cdr state))))

(defun storm-decode (src)
  "Decode as much as possible of a message. Discards the first byte (assumes it is zero). Returns either
   (number result), which means that 'number' characters were consumed resulting in 'result',
   or nil, which means that nothing was consumed."
  (let* ((state (storm-state src))
	 (result (storm-decode-entry src state)))
    (if (storm-state-error state)
	'nil
      (progn
	(cons (car state) result)))))

(defun storm-decode-entry (src state)
  "Decode an entry in (car state) starting at (cdr state)."
  (if (storm-state-more-p state 1)
      (let* ((pos (car state))
	     (ch (aref src pos)))
	(setcar state (1+ pos))
	(cond ((= ch #x00) 'nil)
	      ((= ch #x01) (storm-decode-cons src state))
	      ((= ch #x02) (storm-decode-number src state))
	      ((= ch #x03) (storm-decode-string src state))
	      ((= ch #x04) (storm-decode-new-symbol src state))
	      ((= ch #x05) (storm-decode-old-symbol src state))
	      (t 'nil)))
    (storm-state-set-error state)))

(defun storm-decode-cons (src state)
  "Decode a sequence of cons-cells. If this function would be purely recursive, we quickly run into problems."
  (if (storm-state-error state)
      'nil
    (let* ((first (cons (storm-decode-entry src state) nil))
	   (current first))
      (while (and (storm-state-more-p state 1)
		  (= (aref src (car state)) #x01))
	(setcar state (1+ (car state)))
	(setcdr current (cons
			 (storm-decode-entry src state)
			 nil))
	(setq current (cdr current)))

      ;; Set 'cdr' of the last element as well!
      (if (storm-state-error state)
	  'nil
	(progn
	  (setcdr current (storm-decode-entry src state))
	  first)))))

(defun storm-decode-number (src state)
  "Decode a number"
  (let ((start (car state)))
    (if (storm-state-more-p state 4)
	(progn
	  (setcar state (+ 4 start))
	  (logior (lsh (aref src start) 24)
		  (lsh (aref src (+ start 1)) 16)
		  (lsh (aref src (+ start 2)) 8)
		  (aref src (+ start 3))))
      (storm-state-set-error state))))

(defun storm-decode-string (src state)
  "Decode a string"
  (let ((count (storm-decode-number src state)))
    (if (storm-state-more-p state count)
	(let ((pos (car state)))
	  (setcar state (+ count pos))
	  (decode-coding-string (substring src pos (+ pos count)) 'utf-8))
      (storm-state-set-error state))))

(defun storm-decode-new-symbol (src state)
  "Decode a new symbol"
  (let* ((id (storm-decode-number src state))
	 (str (storm-decode-string src state)))
    (if (storm-state-error state)
	'nil
      (let ((sym (intern str)))
	(puthash sym id storm-process-sym-to-id)
	(puthash id sym storm-process-id-to-sym)
	sym))))

(defun storm-decode-old-symbol (src state)
  "Decode an already existing symbol"
  (let ((id (storm-decode-number src state)))
    (if (storm-state-error state)
	'nil
      (gethash id storm-process-id-to-sym))))

;; Encode messages.

(defun storm-encode (msg)
  "Encode a s-expression."
  (with-temp-buffer
    (set-buffer-multibyte nil)
    (insert-char #x00) ;; Start of message marker.
    (storm-encode-buffer msg)
    (buffer-string)))

(defun storm-encode-buffer (msg)
  "Encode a s-expression. Output to the current buffer."
  (cond ((endp msg)
	 (insert-char #x00))
	((consp msg)
	 (insert-char #x01)
	 (storm-encode-cons msg))
	((numberp msg)
	 (insert-char #x02)
	 (storm-encode-number msg))
	((stringp msg)
	 (insert-char #x03)
	 (storm-encode-string msg))
	((symbolp msg)
	 (storm-encode-sym msg))
	(t 'nil)))

(defun storm-encode-cons (c)
  (storm-encode-buffer (car c))
  (let ((at (cdr c)))
    (while (consp at)
      (insert-char #x01)
      (storm-encode-buffer (car at))
      (setq at (cdr at)))
    (storm-encode-buffer at)))

(defun storm-encode-number (num)
  (insert-char (lsh num -24))
  (insert-char (logand #xFF (lsh num -16)))
  (insert-char (logand #xFF (lsh num -8)))
  (insert-char (logand #xFF num)))

(defun storm-encode-string (str)
  (let ((coded (encode-coding-string str 'utf-8)))
    (storm-encode-number (length coded))
    (insert coded)))

(defun storm-encode-sym (sym)
  (let ((id (gethash sym storm-process-sym-to-id nil)))
    (if id
	(progn
	  (insert #x05)
	  (storm-encode-number id))
      (progn
	(setq id storm-process-next-id)
	(setq storm-process-next-id (1+ id))
	(puthash sym id storm-process-sym-to-id)
	(puthash id sym storm-process-id-to-sym)
	(insert #x04)
	(storm-encode-number id)
	(storm-encode-string (symbol-name sym))))))

;; Send some messages to Storm to test the implementation.
;; (let ((i 0))
;;   (while (< i 2)
;;     (mapcar
;;      (lambda (x)
;;        (send-string storm-process (char-to-string x))
;;        (sleep-for 0 10)
;;        (redisplay))
;;      (storm-encode '(test 1 2 3 4 5 6 7 "Hej")))
;;     (send-string storm-process "garbage")
;;     (setq i (1+ i))))


;; For debugging
;; (setq storm-process-sym-to-id (make-hash-table))
;; (setq storm-process-id-to-sym (make-hash-table))
;; (let ((msg (concat (storm-encode '(color 0 0 1 2 string 2 nil 2 string 2 nil)) "abc")))
;;  (storm-decode-message-string msg))

;; (defun storm-output-string (s z)
;;  (message "%s" s))
;; (recursion-depth)
