# Recovers Table 8.4 (Possible Widgets for Extension) from word positions in the PDF.
import sys, json, re
sys.path.insert(0, r'C:\Users\Steph2\AppData\Local\Temp\claude\C--dev-xml\3a400e3d-0e09-4d44-bfab-3d76f970cff1\scratchpad\pdflib')
import fitz
doc = fitz.open(r'C:\Users\Steph2\Desktop\ARINC_SPECIFICATION_661P1-7.pdf')
result = {}
headers_seen = None
for pno in range(doc.page_count):
    page = doc[pno]
    if pno < 513 or pno > 516:
        continue
    words = page.get_text('words')  # x0,y0,x1,y1,word,block,line,wordno
    # header columns: rotated extension names; take their x centre
    KNOWN = {'DirectionalTabbingExtension','CursorEventsExtension','LegendStringAlignmentExtension','VisibleChildIndexExtension','InitialFocusExtension','FocusStopExtension','CursorShapeExtension','PictureExtension','SymbolExtension','TicsArrayExtension','StyleSetExtension','StaticParamBufferExtension','MultiSelectionExtension','ExcludedRegionsExtension','BoundaryCheckExtension','ReorderExtension','WordWrapExtension','CursorEntryEventsExtension','MapHorzCoordSystemEventExtension','MapVertCoordSystemEventExtension'}
    headers = [(round((w[0] + w[2]) / 2, 1), w[4]) for w in words if w[4] in KNOWN]
    if len(headers) < 15:
        continue
    headers.sort()
    columns = [h[0] for h in headers]
    names = [h[1] for h in headers]
    # widget rows: first word of a line at the left margin that is a widget name
    xs = [w for w in words if w[4] == 'X']
    rows = {}
    for w in words:
        if w[4] != 'X' and not w[4].endswith('Extension') and re.match(r'^[A-Z][A-Za-z_]+$', w[4]) and w[0] < 150:
            rows.setdefault(round((w[1] + w[3]) / 2, 0), []).append(w)
    for ycenter, ws in rows.items():
        name = ''.join(x[4] for x in sorted(ws, key=lambda a: a[0]))
        if name in ('Widgets', 'Extensions', 'WIDGETS', 'ADDED', 'FOR', 'SUPPLEMENT', 'ARINC', 'SPECIFICATION', 'PART', 'Page', 'Table'):
            continue
        marks = []
        for x in xs:
            if abs((x[1] + x[3]) / 2 - ycenter) < 4:
                cx = (x[0] + x[2]) / 2
                col = min(range(len(columns)), key=lambda i: abs(columns[i] - cx))
                if abs(columns[col] - cx) < 12:
                    marks.append(names[col])
        result[name] = sorted(set(marks))
    headers_seen = names
    print('page', pno + 1, 'headers', len(names), 'rows', len(rows), file=sys.stderr)
json.dump({'extensions': headers_seen, 'widgets': result}, open('table84.json', 'w'), indent=1)
print(len(result), 'widgets;', sum(len(v) for v in result.values()), 'marks')
for k in sorted(result)[:8]:
    print(k, result[k])
