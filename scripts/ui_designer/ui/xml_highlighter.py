"""XML syntax highlighter for the Code view editor."""

from PyQt5.QtGui import QSyntaxHighlighter, QTextCharFormat, QColor, QFont
from PyQt5.QtCore import QRegularExpression


class XmlSyntaxHighlighter(QSyntaxHighlighter):
    """Syntax highlighter for XML layout files."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._rules = []

        # XML declaration / processing instruction  <?xml ... ?>
        fmt_pi = QTextCharFormat()
        fmt_pi.setForeground(QColor("#808080"))
        self._rules.append((QRegularExpression(r"<\?.*?\?>"), fmt_pi))

        # Comments  <!-- ... -->
        fmt_comment = QTextCharFormat()
        fmt_comment.setForeground(QColor("#6A9955"))
        fmt_comment.setFontItalic(True)
        self._rules.append((QRegularExpression(r"<!--.*?-->"), fmt_comment))

        # Tag names  <TagName  or </TagName>
        fmt_tag = QTextCharFormat()
        fmt_tag.setForeground(QColor("#569CD6"))
        fmt_tag.setFontWeight(QFont.Bold)
        self._rules.append((QRegularExpression(r"</?[\w:-]+"), fmt_tag))

        # Attribute names  name=
        fmt_attr = QTextCharFormat()
        fmt_attr.setForeground(QColor("#9CDCFE"))
        self._rules.append((QRegularExpression(r'\b[\w:-]+(?=\s*=)'), fmt_attr))

        # Attribute values  "value"
        fmt_value = QTextCharFormat()
        fmt_value.setForeground(QColor("#CE9178"))
        self._rules.append((QRegularExpression(r'"[^"]*"'), fmt_value))
        self._rules.append((QRegularExpression(r"'[^']*'"), fmt_value))

        # Angle brackets and closing slash
        fmt_bracket = QTextCharFormat()
        fmt_bracket.setForeground(QColor("#808080"))
        self._rules.append((QRegularExpression(r"[<>/?]"), fmt_bracket))

    def highlightBlock(self, text):
        for pattern, fmt in self._rules:
            it = pattern.globalMatch(text)
            while it.hasNext():
                match = it.next()
                self.setFormat(match.capturedStart(), match.capturedLength(), fmt)
