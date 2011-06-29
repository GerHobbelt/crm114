;;; crm114-mode.el --- major mode for editing crm114 scripts

;; Copyright (C) 2005  Haavard Kvaalen <havardk@kvaalen.no>

;; Keywords: languages

;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License as
;; published by the Free Software Foundation; either version 2 of the
;; License, or (at your option) any later version.

;; This program is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

;;    To automatically invoke this mode whenever you edit a .crm file,
;;    make sure crm114-mode.el is in your site's default .el directory, 
;;    and add the following to your .emacs file in your home directory
;;    without the ';;' commenting in front, of course!
;;
;;      (load "crm114-mode.el" )
;;      (add-to-list 'auto-mode-alist '("\\.crm\\'" . crm114-mode))

;;
;; $Revision: 1.7 $

(defvar crm114-indent-level 4 
  "Indentation of blocks")

(defvar crm114-mode-syntax-table
  (let ((table (make-syntax-table)))
    (modify-syntax-entry ?#  "< 4"  table)
    (modify-syntax-entry ?\n ">"    table)
    (modify-syntax-entry ?/  "\""   table)
    (modify-syntax-entry ?\\ "\\ 3" table)
    (modify-syntax-entry ?<  "(>"   table)
    (modify-syntax-entry ?>  ")<"   table)
    (modify-syntax-entry ?:  "."    table)
    (modify-syntax-entry ?!  "_"    table)
    (modify-syntax-entry ?\" "_"    table)
    (modify-syntax-entry ?$  "_"    table)
    (modify-syntax-entry ?%  "_"    table)
    (modify-syntax-entry ?&  "_"    table)
    (modify-syntax-entry ?'  "_"    table)
    (modify-syntax-entry ?*  "_"    table)
    (modify-syntax-entry ?+  "_"    table)
    (modify-syntax-entry ?,  "_"    table)
    (modify-syntax-entry ?-  "_"    table)
    (modify-syntax-entry ?.  "_"    table)
    (modify-syntax-entry ?=  "_"    table)
    (modify-syntax-entry ??  "_"    table)
    (modify-syntax-entry ?@  "_"    table)
    (modify-syntax-entry ?^  "_"    table)
    (modify-syntax-entry ?_  "_"    table)
    (modify-syntax-entry ?`  "_"    table)
    (modify-syntax-entry ?|  "_"    table)
    (modify-syntax-entry ?~  "_"    table)
    table))

(defvar crm114-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\t" 'crm114-indent-line)
    (define-key map "}" 'crm114-electric-brace)
    map))

(defvar crm114-font-lock-keywords
  (list
   ;; goto labels
   '("\\(^\\|;\\)\\s-*:\\(\\(:?\\sw\\|\\s_\\)+\\):\\s-*\\($\\|;\\|#\\)" 
     2 'font-lock-constant-face nil)
   ;; functions
   '("\\(^\\|;\\)\\s-*:\\(\\(:?\\sw\\|\\s_\\)+\\):\\s-*(" 
     2 'font-lock-function-name-face nil)
   ;; variables
   '("\\(:\\*\\)?:\\([^: \t\n]+\\):" 2 'font-lock-variable-name-face nil)
   ;; statements
   (list (concat "\\(^\\|;\\)\\s-*"
		 (regexp-opt
		  '("accept" "alius" "alter" "call" "classify" "eval" "exit"
		    "fail" "fault" "goto" "hash" "input" "insert" "intersect"
		    "isolate" "learn" "liaf" "match" "noop" "output" "return"
		    "syscall" "trap" "union" "window")
		  'words))
	 2 'font-lock-keyword-face nil)
   ))

(defvar crm114-font-lock-syntactic-keywords
  (list
   ;; '#' and '/' are allowed within variable names so we need to
   ;; change their syntax at those places.
   '(":[#@]:[^ \t\n]*:\\|:[^: \t\n]*:" 
     ("#\\|/" 
      (progn
	(goto-char (match-beginning 0))
	(setq crm114-end-syntactic (match-end 0)))
      (goto-char crm114-end-syntactic)
      (0 "_")))))

(defun crm114-end-of-line ()
  (save-excursion
    (end-of-line)
    (point)))

(defun crm114-calculate-indent ()
  "Calculate and return indentation for the current line."
  (save-excursion
    (beginning-of-line)
    (let (ret cont
	  (n 0)
	  (bol (point)))
      (while (looking-at "[ \t]*}")
	(goto-char (match-end 0))
	(setq n (1- n)))
      ;; Find last line that is not empty and is not all comment
      (while (and (or (= (forward-line -1) 0)
		      (progn
			(setq ret 0)
			nil))
		  (looking-at "[ \t]*\\(#\\|$\\)")))
      (crm114-beginning-of-syntax)
      (or
       ret
       (let ((indent (current-indentation))
	     (end (crm114-end-of-line))
	     (search-string (concat "\\(#\\)\\|\\(/\\)\\|\\(:\\)\\|"
				    "\\({\\)\\|\\(}\\)\\|\\(\\\\\\)?$")))
	 ;; Leading closing brace affects previous line
	 (while (looking-at "[ \t]*}")
	   (goto-char (match-end 0)))
	 (while (and (re-search-forward search-string end t)
		     (cond ((match-beginning 1) ; comment
			    (re-search-forward "\\\\#" end t))
			   ((match-beginning 2) ; string 
			    ;; The extra paranthesis are here to work
			    ;; around what seems to be a bug seen on
			    ;; xemacs 21.4.6 (on debian)
			    (while (and (not (and (looking-at "/\\|\\(.*?[^\\\n]\\)/")
						  (goto-char (match-end 0))))
					(forward-line 1)
					(setq end (crm114-end-of-line))
					(or (< (point) bol)
					    (not (setq ret 0)))))
			    t)
			   ((match-beginning 3) ; variable
			    (goto-char (match-beginning 3))
			    (crm114-skip-variable end))
			   ((match-beginning 4) ; opening brace
			    (setq n (1+ n)))
			   ((match-beginning 5) ; closing brace
			    (setq n (1- n)))
			   (t			; eol
			    (setq cont (match-beginning 6))
			    (forward-line 1)
			    (setq end (crm114-end-of-line))))
		     (< (point) bol)
		     (not ret)))
	 (or ret
	     (progn
	       (when cont
		 (setq n (+ n 2)))
	       (+ indent (* n crm114-indent-level)))))))))


(defun crm114-indent-line ()
  (interactive)
  (let (beg
	(pos (- (point-max) (point)))
	(indent (crm114-calculate-indent)))
    (beginning-of-line)
    (setq beg (point))
    (skip-chars-forward " \t")
    (delete-region beg (point))
    (indent-to (crm114-calculate-indent))
    (if (> (- (point-max) pos) (point))
	(goto-char (- (point-max) pos)))))

(defun crm114-electric-brace (arg)
  (interactive "p")
  (if (> arg 0)
      (progn
	(insert-char last-command-char arg)
	(crm114-indent-line)
	(delete-char (- arg))
	(self-insert-command arg))))

(defun crm114-skip-variable (max)
  (unless (looking-at ":")
    (error "Not at variable start"))
  (if (looking-at ":[#@]:\\([^ \t\n]*\\)\\(:\\)")
      (progn
	(goto-char (match-beginning 1))
	(let ((last (match-beginning 2))
	      (end (match-end 2)))
	  (when (re-search-forward ":" last t)
	    (goto-char (match-beginning 0))
	    (crm114-skip-variable last))
	  (goto-char end)))
    (if (looking-at ":\\*:[^ \t\n:]*:")
	(goto-char (match-end 0))
      (goto-char (1+ (point)))
      (re-search-forward ":" max 'to-end))))

(defun crm114-beginning-of-syntax ()
  "Go backwards until the start of the current statement."
  (beginning-of-line)
  (when (> (- (point) (point-min)) 1)
    (let ((pos (point)))
      (goto-char (- (point) 2))
      (if (looking-at "\\\\$")
	  (progn
	    (beginning-of-line)
	    (while (and (or (re-search-forward "\\(#\\)\\|\\(/\\)\\|\\(:\\)" pos t)
			    (progn
			      (crm114-beginning-of-syntax)
			      nil))
			(cond ((match-beginning 1)
			       (if (re-search-forward "\\\\#" pos t)
				   t
				 (goto-char pos)
				 nil))
			      ((match-beginning 2)
			       (if (looking-at "/\\|.*?[^\\\n]/")
				   (goto-char (match-end 0))
				 (crm114-beginning-of-syntax)
				 nil))
			      ((match-beginning 3)
			       (goto-char (match-beginning 3))
			       (crm114-skip-variable pos)
			       t)))))
	(goto-char pos)))))

		  

(defun crm114-mode ()
  "Major mode for editing crm scripts.  

CRM114, also known as The Controllable Regex Mutilator is a language
designed for implementation of contextual filters.

Turning on crm114 mode runs `crm114-mode-hook'."
  (interactive)
  (kill-all-local-variables)
  (setq mode-name "crm114")
  (setq major-mode 'crm114-mode)
  (use-local-map crm114-mode-map)
  (set-syntax-table crm114-mode-syntax-table)
  (make-local-variable 'indent-line-function)
  (setq indent-line-function 'crm114-indent-line)
  (setq font-lock-defaults 
	'(crm114-font-lock-keywords 
	  nil nil nil
	  crm114-beginning-of-syntax
	  (font-lock-syntactic-keywords . crm114-font-lock-syntactic-keywords)))
  (make-local-variable 'comment-start)
  (setq comment-start "# ")
  (make-local-variable 'comment-end)
  (setq comment-end "")
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip "#+[ \t]*")
  (run-hooks 'crm114-mode-hook))

(provide 'crm114-mode)
