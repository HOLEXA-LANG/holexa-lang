;;; holexa-mode.el --- Major mode for HOLEXA programming language

(defvar holexa-mode-hook nil)

(defvar holexa-keywords
  '("let" "mut" "fn" "return" "if" "else" "while" "for" "in" "loop"
    "break" "continue" "struct" "impl" "trait" "import" "use" "pub"
    "match" "enum" "async" "await" "const" "static" "self" "true" "false" "none"))

(defvar holexa-types
  '("int" "float" "String" "bool" "void" "byte" "char"
    "i8" "i16" "i32" "i64" "u8" "u16" "u32" "u64" "f32" "f64" "Array" "Map"))

(defvar holexa-font-lock-keywords
  `((,(regexp-opt holexa-keywords 'words) . font-lock-keyword-face)
    (,(regexp-opt holexa-types 'words) . font-lock-type-face)
    ("\"[^\"]*\"" . font-lock-string-face)
    ("//.*$" . font-lock-comment-face)
    ("[a-z_][a-zA-Z0-9_]*\\s-*(" 1 font-lock-function-name-face)))

(define-derived-mode holexa-mode prog-mode "HOLEXA"
  "Major mode for editing HOLEXA source code."
  (setq font-lock-defaults '(holexa-font-lock-keywords))
  (setq comment-start "// ")
  (setq comment-end ""))

(add-to-list 'auto-mode-alist '("\\.hlx\\'" . holexa-mode))
(provide 'holexa-mode)
;;; holexa-mode.el ends here
