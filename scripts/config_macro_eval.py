#!/usr/bin/env python3
"""Helpers for evaluating integer config macros from local app headers."""

from __future__ import annotations

import ast
import re
import shlex
from pathlib import Path


_INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s+"([^"]+)"\s*$')
_DEFINE_PATTERN = re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)(?:\s+(.+?))?\s*$')
_UNDEF_PATTERN = re.compile(r'^\s*#\s*undef\s+([A-Za-z_]\w*)\s*$')
_IFDEF_PATTERN = re.compile(r'^\s*#\s*ifdef\s+([A-Za-z_]\w*)\s*$')
_IFNDEF_PATTERN = re.compile(r'^\s*#\s*ifndef\s+([A-Za-z_]\w*)\s*$')
_IF_PATTERN = re.compile(r'^\s*#\s*if\s+(.+?)\s*$')
_ELIF_PATTERN = re.compile(r'^\s*#\s*elif\s+(.+?)\s*$')
_ELSE_PATTERN = re.compile(r'^\s*#\s*else\b')
_ENDIF_PATTERN = re.compile(r'^\s*#\s*endif\b')
_DEFINED_PATTERN = re.compile(r'\bdefined\s*(?:\(\s*([A-Za-z_]\w*)\s*\)|([A-Za-z_]\w*))')
_IDENTIFIER_PATTERN = re.compile(r'\b[A-Za-z_]\w*\b')
_INT_SUFFIX_PATTERN = re.compile(r'\b(0[xX][0-9A-Fa-f]+|\d+)([uUlL]+)\b')
_BLOCK_COMMENT_PATTERN = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE_COMMENT_PATTERN = re.compile(r"//.*?$", re.MULTILINE)
_DEFINE_TOKEN_PATTERN = re.compile(r"^[A-Za-z_]\w*(?:=.*)?$", re.DOTALL)
_SAFE_VALUE_EXPR_PATTERN = re.compile(r'^[0-9A-Fa-fxX_ \t()+\-*/%<>&|^~]+$')
_SAFE_CONDITION_EXPR_PATTERN = re.compile(r'^[0-9A-Za-z-fxX_ \t()+\-*/%<>&|^~=!]+$')
_ALLOWED_BINOPS = (
    ast.Add,
    ast.Sub,
    ast.Mult,
    ast.FloorDiv,
    ast.Mod,
    ast.LShift,
    ast.RShift,
    ast.BitOr,
    ast.BitAnd,
    ast.BitXor,
)
_ALLOWED_UNARYOPS = (ast.UAdd, ast.USub, ast.Invert, ast.Not)
_ALLOWED_CMPOPS = (ast.Eq, ast.NotEq, ast.Lt, ast.LtE, ast.Gt, ast.GtE)
_PYTHON_KEYWORDS = {"and", "or", "not", "True", "False"}


class MacroEvalError(ValueError):
    """Raised when a config macro expression cannot be resolved safely."""


def _strip_comments(text: str) -> str:
    text = _BLOCK_COMMENT_PATTERN.sub("", text)
    return _LINE_COMMENT_PATTERN.sub("", text)


def _normalize_text(text: str) -> str:
    return _strip_comments(text).replace("\\\r\n", "").replace("\\\n", "")


def _split_cflags(user_cflags: str | None) -> list[str]:
    if not user_cflags:
        return []
    try:
        return shlex.split(user_cflags, posix=False)
    except ValueError:
        return user_cflags.split()


def _strip_wrapping_quotes(value: str) -> str:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    return value


def parse_macro_defines_from_cflags(user_cflags: str | None) -> dict[str, str]:
    """Parse integer-style compiler -D defines from a USER_CFLAGS string."""
    defines: dict[str, str] = {}
    tokens = _split_cflags(user_cflags)
    index = 0
    while index < len(tokens):
        token = tokens[index]
        define_spec = None
        if token in ("-D", "/D"):
            index += 1
            if index < len(tokens):
                define_spec = tokens[index]
        elif token.startswith("-D") and len(token) > 2:
            define_spec = token[2:]
        elif token.startswith("/D") and len(token) > 2:
            define_spec = token[2:]

        if define_spec:
            define_spec = _strip_wrapping_quotes(define_spec.strip())
            if _DEFINE_TOKEN_PATTERN.match(define_spec):
                name, has_value, value = define_spec.partition("=")
                defines[name] = _strip_wrapping_quotes(value.strip()) if has_value else "1"
        index += 1
    return defines


def _validate_ast(node: ast.AST) -> None:
    if isinstance(node, ast.Expression):
        _validate_ast(node.body)
        return
    if isinstance(node, ast.Constant):
        if not isinstance(node.value, int):
            raise MacroEvalError("only integer constants are supported")
        return
    if isinstance(node, ast.BinOp):
        if not isinstance(node.op, _ALLOWED_BINOPS):
            raise MacroEvalError(f"unsupported operator: {type(node.op).__name__}")
        _validate_ast(node.left)
        _validate_ast(node.right)
        return
    if isinstance(node, ast.UnaryOp):
        if not isinstance(node.op, _ALLOWED_UNARYOPS):
            raise MacroEvalError(f"unsupported unary operator: {type(node.op).__name__}")
        _validate_ast(node.operand)
        return
    if isinstance(node, ast.BoolOp):
        if not isinstance(node.op, (ast.And, ast.Or)):
            raise MacroEvalError(f"unsupported boolean operator: {type(node.op).__name__}")
        for value in node.values:
            _validate_ast(value)
        return
    if isinstance(node, ast.Compare):
        _validate_ast(node.left)
        for op in node.ops:
            if not isinstance(op, _ALLOWED_CMPOPS):
                raise MacroEvalError(f"unsupported compare operator: {type(op).__name__}")
        for comparator in node.comparators:
            _validate_ast(comparator)
        return
    raise MacroEvalError(f"unsupported AST node: {type(node).__name__}")


def _safe_eval_int(expr: str) -> int:
    try:
        parsed = ast.parse(expr, mode="eval")
    except SyntaxError as exc:
        raise MacroEvalError(f"invalid expression: {expr}") from exc
    _validate_ast(parsed)
    return int(eval(compile(parsed, "<config-macro>", "eval"), {"__builtins__": {}}, {}))


def _resolve_identifier(ident: str, owner: str | None, defines: dict[str, str], cache: dict[str, int], visiting: set[str], unknown_default: int | None) -> int:
    if owner is not None and ident == owner:
        raise MacroEvalError(f"self-referential macro: {owner}")
    if ident not in defines:
        if unknown_default is not None:
            return unknown_default
        raise MacroEvalError(f"unknown identifier in {owner}: {ident}" if owner is not None else f"unknown macro: {ident}")
    return _resolve_macro(ident, defines, cache, visiting, unknown_default)


def _resolve_macro(name: str, defines: dict[str, str], cache: dict[str, int], visiting: set[str], unknown_default: int | None = None) -> int:
    if name in cache:
        return cache[name]
    if name in visiting:
        raise MacroEvalError(f"cyclic macro reference: {name}")
    if name not in defines:
        if unknown_default is not None:
            return unknown_default
        raise MacroEvalError(f"unknown macro: {name}")

    visiting.add(name)
    try:
        expr = _INT_SUFFIX_PATTERN.sub(r"\1", defines[name])

        def replace_identifier(match: re.Match[str]) -> str:
            ident = match.group(0)
            if ident in _PYTHON_KEYWORDS:
                return ident
            return f"({_resolve_identifier(ident, name, defines, cache, visiting, unknown_default)})"

        expr = _IDENTIFIER_PATTERN.sub(replace_identifier, expr)
        expr = expr.replace("/", "//")
        if not _SAFE_VALUE_EXPR_PATTERN.fullmatch(expr):
            raise MacroEvalError(f"unsafe expression: {expr}")

        value = _safe_eval_int(expr)
        cache[name] = value
        return value
    finally:
        visiting.remove(name)


def _eval_condition_expr(expr: str, defines: dict[str, str], cache: dict[str, int]) -> bool:
    expr = _INT_SUFFIX_PATTERN.sub(r"\1", expr)
    expr = _DEFINED_PATTERN.sub(lambda match: "1" if (match.group(1) or match.group(2)) in defines else "0", expr)
    expr = expr.replace("&&", " and ").replace("||", " or ")
    expr = re.sub(r'!(?!=)', ' not ', expr)

    def replace_identifier(match: re.Match[str]) -> str:
        ident = match.group(0)
        if ident in _PYTHON_KEYWORDS:
            return ident
        return f"({_resolve_identifier(ident, None, defines, cache, set(), 0)})"

    expr = _IDENTIFIER_PATTERN.sub(replace_identifier, expr)
    expr = expr.replace("/", "//")
    if not _SAFE_CONDITION_EXPR_PATTERN.fullmatch(expr):
        raise MacroEvalError(f"unsafe condition expression: {expr}")
    return bool(_safe_eval_int(expr))


def _collect_defines_from_file(config_path: Path, defines: dict[str, str], cache: dict[str, int], stack: set[Path] | None = None) -> None:
    path = Path(config_path).resolve()
    visited = stack if stack is not None else set()
    if path in visited:
        return

    try:
        raw_text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise MacroEvalError(f"failed to read {path}: {exc}") from exc

    visited.add(path)
    condition_stack: list[dict[str, bool]] = []
    current_active = True

    for line in _normalize_text(raw_text).splitlines():
        stripped = line.strip()
        if not stripped:
            continue

        ifdef_match = _IFDEF_PATTERN.match(line)
        if ifdef_match is not None:
            cond = ifdef_match.group(1) in defines
            condition_stack.append({"parent_active": current_active, "taken": cond, "active": current_active and cond})
            current_active = condition_stack[-1]["active"]
            continue

        ifndef_match = _IFNDEF_PATTERN.match(line)
        if ifndef_match is not None:
            cond = ifndef_match.group(1) not in defines
            condition_stack.append({"parent_active": current_active, "taken": cond, "active": current_active and cond})
            current_active = condition_stack[-1]["active"]
            continue

        if_match = _IF_PATTERN.match(line)
        if if_match is not None:
            cond = _eval_condition_expr(if_match.group(1), defines, cache) if current_active else False
            condition_stack.append({"parent_active": current_active, "taken": cond, "active": current_active and cond})
            current_active = condition_stack[-1]["active"]
            continue

        elif_match = _ELIF_PATTERN.match(line)
        if elif_match is not None:
            if not condition_stack:
                continue
            frame = condition_stack[-1]
            if frame["parent_active"] and not frame["taken"]:
                cond = _eval_condition_expr(elif_match.group(1), defines, cache)
                frame["active"] = cond
                frame["taken"] = cond
            else:
                frame["active"] = False
            current_active = frame["parent_active"] and frame["active"]
            continue

        if _ELSE_PATTERN.match(line) is not None:
            if not condition_stack:
                continue
            frame = condition_stack[-1]
            frame["active"] = frame["parent_active"] and not frame["taken"]
            frame["taken"] = True
            current_active = frame["active"]
            continue

        if _ENDIF_PATTERN.match(line) is not None:
            if not condition_stack:
                continue
            frame = condition_stack.pop()
            current_active = frame["parent_active"]
            continue

        if not current_active:
            continue

        include_match = _INCLUDE_PATTERN.match(line)
        if include_match is not None:
            include_path = (path.parent / include_match.group(1)).resolve()
            if include_path.exists():
                _collect_defines_from_file(include_path, defines, cache, visited)
            continue

        undef_match = _UNDEF_PATTERN.match(line)
        if undef_match is not None:
            defines.pop(undef_match.group(1), None)
            cache.clear()
            continue

        define_match = _DEFINE_PATTERN.match(line)
        if define_match is not None:
            defines[define_match.group(1)] = (define_match.group(2) or "1").strip()
            cache.clear()

    visited.remove(path)


def get_macro_int_from_config(config_path: Path | str, macro_name: str, default=None, initial_defines=None, user_cflags=None):
    """Resolve one integer-like macro from a config header and its local includes."""
    path = Path(config_path)
    if not path.exists():
        return default

    try:
        defines: dict[str, str] = {}
        if initial_defines:
            defines.update({str(key): str(value) for key, value in initial_defines.items()})
        defines.update(parse_macro_defines_from_cflags(user_cflags))
        cache: dict[str, int] = {}
        _collect_defines_from_file(path, defines, cache)
        if macro_name not in defines:
            return default
        return _resolve_macro(macro_name, defines, cache, set())
    except (MacroEvalError, ArithmeticError):
        return default
