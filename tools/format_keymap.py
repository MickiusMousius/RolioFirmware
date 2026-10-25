import re
import sys

MIN_COL_WIDTH = 17
LAYOUT_SIZES = (46, 48)
# Box drawing overlaps 2 characters at the edge of some cells, keep 1 more clear so bindings don't touch it
MARKER_PAD = 3

# Grid columns from left to right. L6 and R-1 are the inner columns between the halves,
# used by the inner thumb keys and (on the 48 key layout) the encoder push keys.
COLUMNS = [('L', c) for c in range(7)] + [('R', c) for c in range(-1, 6)]


def parse_bindings(text):
    text = re.sub(r'/\*.*?\*/', ' ', text)
    text = re.sub(r'//.*', ' ', text)
    tokens = text.split()
    bindings = []
    current = []
    for token in tokens:
        is_start = token.startswith('&') or token == '___' or token.endswith('_PAIR')
        if is_start:
            if current: bindings.append(' '.join(current))
            current = [token]
        else:
            if not current: current = [token]
            else: current.append(token)
    if current: bindings.append(' '.join(current))
    return bindings


def cell_layout(count):
    """
    Returns a list of (row, column, pad_left, pad_right) for each binding position.
    The pads are the number of characters at each edge of a cell that must stay clear of the box drawing.
    """
    p = MARKER_PAD
    cells = []
    for r in range(3):
        right_row_start = len(cells) + 6
        for c in range(6):
            cells.append((r, ('L', c), p if c == 0 else 0, p if c == 5 else 0))
        if count == 48 and r == 2:
            # Encoder push keys sit between the halves on the bottom row
            cells.append((r, ('L', 6), 0, 0))
            cells.append((r, ('R', -1), 0, 0))
        for c in range(6):
            cells.append((r, ('R', c), p if c == 0 else 0, p if c == 5 else 0))
    for i, c in enumerate([2, 3, 4, 5, 6]):
        cells.append((3, ('L', c), p if i == 0 else 0, 0))
    for i, c in enumerate([-1, 0, 1, 2, 3]):
        cells.append((3, ('R', c), 0, p if i == 4 else 0))
    return cells


def column_widths(blocks):
    """Size each column to fit the longest binding found in it, across all layers."""
    widths = {col: MIN_COL_WIDTH for col in COLUMNS}
    for bindings in blocks:
        for binding, (_, col, pad_l, pad_r) in zip(bindings, cell_layout(len(bindings))):
            widths[col] = max(widths[col], len(binding) + pad_l + pad_r)
    return widths


class Canvas:
    def __init__(self, width, height=7):
        self.lines = [list(" " * width) for _ in range(height)]
    def write(self, row, col, text):
        for i, c in enumerate(text):
            if 0 <= col + i < len(self.lines[row]):
                self.lines[row][col + i] = c
    def get_lines(self):
        return ["".join(l).rstrip() for l in self.lines if "".join(l).strip()]


def format_block(bindings, widths):
    if len(bindings) not in LAYOUT_SIZES: return None

    L_start = 5
    inner_gap = 8
    starts = {}
    pos = L_start
    for col in COLUMNS:
        if col == ('R', -1):
            pos += inner_gap - 1
        starts[col] = pos
        pos += widths[col] + 1

    def C(side, col): return starts[(side, col)]
    def w(side, col): return widths[(side, col)]

    def box(s, width, pad_l, pad_r):
        # Centre the binding, then nudge it away from any box drawing that overlaps the cell edges
        offset = len(s.center(width)) - len(s.center(width).lstrip()) if s.strip() else 0
        offset = max(pad_l, min(offset, width - pad_r - len(s)))
        return " " * offset + s

    canvas = Canvas(pos + 10)

    # Top border
    canvas.write(0, 0, "//")
    for side in ('L', 'R'):
        canvas.write(0, C(side, 0) - 1, "╭")
        for c in range(6):
            canvas.write(0, C(side, c), "─" * w(side, c))
            canvas.write(0, C(side, c) + w(side, c), "┬" if c < 5 else "╮")

    # Bindings
    for binding, (r, (side, c), pad_l, pad_r) in zip(bindings, cell_layout(len(bindings))):
        canvas.write(r + 1, C(side, c), box(binding, w(side, c), pad_l, pad_r))

    # The inner thumb column starts on the bottom row of keys, or the row above if the encoders are on the bottom row
    inner_top_row = 2 if len(bindings) == 48 else 3
    for r in range(3):
        canvas_row = r + 1
        canvas.write(canvas_row, C('L', 0) - 3, "/*|*/")
        canvas.write(canvas_row, C('R', 5) + w('R', 5) - 2, "/*|*/")
        if canvas_row < inner_top_row:
            canvas.write(canvas_row, C('L', 5) + w('L', 5) - 2, "/*|")
            canvas.write(canvas_row, C('R', 0) - 1, "|*/")
        elif canvas_row == inner_top_row:
            canvas.write(canvas_row, C('L', 5) + w('L', 5) - 2, "/*├")
            canvas.write(canvas_row, C('L', 6), "─" * w('L', 6))
            canvas.write(canvas_row, C('L', 6) + w('L', 6), "╮")
            canvas.write(canvas_row, C('R', -1) - 1, "╭")
            canvas.write(canvas_row, C('R', -1), "─" * w('R', -1))
            canvas.write(canvas_row, C('R', 0) - 1, "┤*/")

    # Thumb row
    canvas_row = 4
    canvas.write(canvas_row, C('L', 0) - 3, "/*╰")
    canvas.write(canvas_row, C('L', 0), "─" * w('L', 0))
    canvas.write(canvas_row, C('L', 0) + w('L', 0), "┴")
    canvas.write(canvas_row, C('L', 1), "─" * w('L', 1))
    canvas.write(canvas_row, C('L', 1) + w('L', 1), "┴*/")
    canvas.write(canvas_row, C('R', 4) - 3, "/*┴")
    canvas.write(canvas_row, C('R', 4), "─" * w('R', 4))
    canvas.write(canvas_row, C('R', 4) + w('R', 4), "┴")
    canvas.write(canvas_row, C('R', 5), "─" * w('R', 5))
    canvas.write(canvas_row, C('R', 5) + w('R', 5), "╯*/")

    # Bottom border
    canvas_row = 5
    canvas.write(canvas_row, 0, "//")
    canvas.write(canvas_row, C('L', 2) - 1, "╰")
    for c in range(2, 7):
        canvas.write(canvas_row, C('L', c), "─" * w('L', c))
        canvas.write(canvas_row, C('L', c) + w('L', c), "┴" if c < 6 else "╯")
    canvas.write(canvas_row, C('R', -1) - 1, "╰")
    for c in range(-1, 4):
        canvas.write(canvas_row, C('R', c), "─" * w('R', c))
        canvas.write(canvas_row, C('R', c) + w('R', c), "┴" if c < 3 else "╯")
    return "\n".join(canvas.get_lines())


BINDINGS_RE = re.compile(r'bindings\s*=\s*<\s*(.*?)\s*>;', flags=re.DOTALL)


def process_file(path):
    with open(path, 'r') as f: content = f.read()

    content = re.sub(r'\n[ \t]*\n[ \t]*\n+', '\n\n', content)
    content = re.sub(r'^      ([a-z0-9_]+ \{)', r'        \1', content, flags=re.MULTILINE)
    content = re.sub(r'^      (};)', r'        \1', content, flags=re.MULTILINE)

    blocks = [b for b in (parse_bindings(m.group(1)) for m in BINDINGS_RE.finditer(content))
              if len(b) in LAYOUT_SIZES]
    widths = column_widths(blocks)

    def replacer(match):
        bindings = parse_bindings(match.group(1))
        formatted = format_block(bindings, widths)
        if formatted: return "bindings = <\n" + formatted + "\n            >;"
        return match.group(0)
    new_content = BINDINGS_RE.sub(replacer, content)
    with open(path, 'w') as f: f.write(new_content)


if __name__ == "__main__":
    process_file(sys.argv[1])
