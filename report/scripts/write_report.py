"""Render report/report.md to PDF without modifying the Markdown source."""
from pathlib import Path
import re
from xml.sax.saxutils import escape

from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'report/report.md'
OUTPUT = ROOT / 'output/pdf/movie-catalog-report.pdf'


def inline(text):
    """Support common inline Markdown while escaping literal XML characters."""
    tokens = []

    def stash(markup):
        tokens.append(markup)
        return f'\x00{len(tokens) - 1}\x00'

    text = re.sub(r'`([^`]+)`', lambda m: stash(
        '<font name="Courier">' + escape(m[1]) + '</font>'), text)
    text = re.sub(r'\[([^\]]+)\]\(([^\s)]+)\)', lambda m: stash(
        '<link href="' + escape(m[2], {'"': '&quot;'}) + '">' + escape(m[1]) + '</link>'), text)
    text = escape(text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)
    text = re.sub(r'(?<!\*)\*([^*]+)\*(?!\*)', r'<i>\1</i>', text)
    return re.sub(r'\x00(\d+)\x00', lambda m: tokens[int(m[1])], text)


def styles():
    result = getSampleStyleSheet()
    for name, options in {
        'BodyReport': dict(fontName='Times-Roman', fontSize=11, leading=13.5, spaceAfter=7),
        'HeadingReport': dict(fontName='Times-Roman', fontSize=17, leading=20,
                              textColor=colors.HexColor('#12506b'), spaceBefore=8,
                              spaceAfter=10, keepWithNext=True),
        'CoverReport': dict(fontName='Times-Bold', fontSize=29, leading=35,
                            alignment=TA_CENTER, spaceAfter=22),
        'CenterReport': dict(fontName='Times-Roman', fontSize=13, leading=19,
                             alignment=TA_CENTER, spaceAfter=14),
        'CellReport': dict(fontName='Times-Roman', fontSize=9, leading=11),
    }.items():
        result.add(ParagraphStyle(name=name, **options))
    return result


def table(rows, sheet):
    count = len(rows[0])
    if any(len(row) != count for row in rows):
        raise ValueError('Markdown table rows must have the same number of columns')
    widths = {3: [133, 171, 164], 4: [90, 126, 126, 126]}.get(count, [468 / count] * count)
    cells = [[Paragraph(inline(cell), sheet['CellReport']) for cell in row] for row in rows]
    result = Table(cells, colWidths=widths, repeatRows=1, hAlign='LEFT')
    result.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#e9f0f3')),
        ('GRID', (0, 0), (-1, -1), .45, colors.HexColor('#687780')),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('LEFTPADDING', (0, 0), (-1, -1), 6),
        ('RIGHTPADDING', (0, 0), (-1, -1), 6),
        ('TOPPADDING', (0, 0), (-1, -1), 6),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 6),
    ]))
    return result


def flowables(markdown):
    sheet = styles()
    story = []
    cover = True
    lines = markdown.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        index += 1
        if not line:
            continue
        if line == '<!-- page break -->':
            story.append(PageBreak())
            cover = False
        elif line.startswith('# '):
            story.extend([Spacer(1, 100), Paragraph(inline(line[2:]), sheet['CoverReport'])])
        elif line.startswith('## '):
            cover = False
            story.append(Paragraph(inline(line[3:]), sheet['HeadingReport']))
        elif '|' in line:
            rows = []
            while True:
                cells = [cell.strip() for cell in line.strip('|').split('|')]
                if not all(re.fullmatch(r':?-{3,}:?', cell) for cell in cells):
                    rows.append(cells)
                if index >= len(lines) or '|' not in lines[index]:
                    break
                line = lines[index].strip()
                index += 1
            story.extend([table(rows, sheet), Spacer(1, 9)])
        else:
            bullet = re.match(r'^(?:[-*]|\d+\.)\s+', line)
            if bullet:
                story.append(Paragraph(inline(line[bullet.end():]), sheet['BodyReport'],
                                       bulletText='\u2022' if line[0] in '-*' else line[:bullet.end()].strip()))
                continue
            paragraph = [line]
            while index < len(lines) and lines[index].strip():
                next_line = lines[index].strip()
                if next_line.startswith(('#', '<!--', '- ', '* ')) or '|' in next_line:
                    break
                paragraph.append(next_line)
                index += 1
            story.append(Paragraph(inline(' '.join(paragraph)),
                                   sheet['CenterReport' if cover else 'BodyReport']))
    return story


def footer(canvas, doc):
    if doc.page > 1:
        canvas.setFont('Times-Roman', 9)
        canvas.drawString(72, 36, 'Movie Catalog Search | Fall 2026')
        canvas.drawRightString(540, 36, str(doc.page - 1))


def main():
    story = flowables(SOURCE.read_text())
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    SimpleDocTemplate(str(OUTPUT), pagesize=(612, 792), leftMargin=72, rightMargin=72,
                      topMargin=49, bottomMargin=49).build(story, onLaterPages=footer)
    print(f'Created {OUTPUT}')


if __name__ == '__main__':
    main()
