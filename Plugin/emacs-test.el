;; Tests for the emacs plugin (mainly the protocol).
(load "emacs.el")

;; Run tests using C-c t
(global-set-key (kbd "C-c t") 'storm-run-tests)

(defun storm-run-tests ()
  (interactive)
  (puthash 'test 'storm-on-test storm-messages)
  (add-hook 'storm-started-hook 'storm-start-tests)
  (storm-restart))

;; All tests to run.
(defvar storm-tests
  '(storm-short-messages)
  "All tests to execute.")
(defvar storm-test-msg nil "Store any test messages arrived.")


(defface storm-test-msg
  '((t :foreground "dark green"))
  "Face used indicating test status.")

(defface storm-test-fail
  '((t :foreground "red"))
  "Face used indicating test failures.")

(defun storm-start-tests ()
  "Called when a new instance has been started."
  (let ((at storm-tests)
	(ok t))
    (while (consp at)
      (storm-output-string (format "Running %S...\n" (car at)) 'storm-test-msg)
      (if (funcall (car at))
	  (setq at (cdr at))
	(progn
	  (storm-output-string (format "\nFailed %S. Terminating.\n" (car at)) 'storm-test-fail)
	  (setq ok nil)
	  (setq at nil))))

    (when ok
      (storm-output-string "\nAll tests passed!\n" 'storm-test-msg))))

(defun storm-wait-message ()
  "Wait for the storm process to send us a message. Times out after about a second."
  (let ((result t))
    (while (and result
		(endp storm-test-msg))
      (redisplay)
      (setq result (accept-process-output storm-process 1)))

    (prog1
	storm-test-msg
      (setq storm-test-msg nil))))



(defun storm-short-messages ()
  "Send lots of short messages to Storm."
  (storm-send '(test start))
  (let ((msg '(test sum 1 2 3))
	(times 100))
    (dotimes (i times)
      (storm-send msg))

    (storm-send '(test stop))
    (equal (storm-wait-message)
	   (list 'sum (* (+ 1 2 3) times)))))


(defun storm-on-test (msg)
  "Called when a test-message has arrived."
  (if (consp msg)
      (progn
	(setq storm-test-msg msg)
	t)
    "Test messages should be lists."))

