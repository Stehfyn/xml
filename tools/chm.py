# Generates docs/help/*.htm, the HTML Help project, and docs/arinc661.chm from help.json (needs HTML Help Workshop hhc.exe).
# Usage: python tools/chm.py   (run in the folder holding help.json, after tools/help.py)
import json, html, os, subprocess
w = json.load(open('help.json'))
root = r'C:\dev\xml\docs\help'
os.makedirs(root, exist_ok=True)
face = [("Configuration", "The FACE display management configuration file (FACE Technical Standard, Appendix H.3): screens, layouts, windows, user applications, and external sources. Create one with File > New Configuration, add a screen, a layout, a window, then a user application pointing at a Definition File."),
        ("Screen", "A physical display surface (H.3.3 to H.3.5): pixelSize gives its resolution, physicalDimensions its size in mm or inches. A screen holds layouts. The editor uses pixelSize over physicalDimensions as the pixel density for previews when a window has no pixel-per-mm scaling factor."),
        ("Layout", "A set of windows that are shown together on one screen (H.3.6). Select a layout to see and add its windows; a window belongs to exactly one layout."),
        ("Window", "A rectangle of a screen, in pixels (H.3.2): pixelArea is its origin and size, scalingFactor converts ARINC 661 screen units to pixels. User applications target windows by id. Drag a window in the scene to move it, drag its corners to resize."),
        ("UserApplication", "An ARINC 661 User Application (H.3.1): applicationId must equal the ApplicationId inside its Definition File, window names the window its layers appear in, dFPath names the Definition File. The inspector checks the identifier against the open Definition File and previews its layers inside the window."),
        ("ExternalSource", "An external video or graphics source (H.3.7) that an ExternalSource widget can show by SourceReference.")]
style = '<style>body{font-family:Segoe UI,Arial,sans-serif;margin:1.5em;color:#222}table{border-collapse:collapse;width:100%}td,th{border:1px solid #ddd;padding:4px 6px;vertical-align:top;font-size:0.95em}th{background:#eee;text-align:left}.cat{color:#666}</style>'
def page(title, body):
    return '<!DOCTYPE html><html><head><meta charset="utf-8"><title>%s</title>%s</head><body>%s</body></html>' % (html.escape(title), style, body)
files = []
def write(name, title, body):
    open(os.path.join(root, name), 'w', encoding='utf-8').write(page(title, body))
    files.append(name)
write('Getting-Started.htm', 'Getting started', '<h1>Getting started</h1><ol>'
 '<li><b>Configuration</b>: File &gt; New Configuration. Add a Screen, a Layout, a Window (toolbar or Edit menu); set the window pixelArea and scalingFactor. Add a User Application, choose its Window and its DF Path.</li>'
 '<li><b>Definition File</b>: File &gt; New Definition File. Add a Layer, then right-click the scene to add widgets at the click point, or use the toolbar. Containers accept children: right-click a container, or drag a node onto it in the hierarchy.</li>'
 '<li><b>Scene</b>: drag to move, corner handles resize, arrows nudge (Shift x10), Delete removes, wheel zooms, middle-drag or Space-drag pans, F fits, G grid, S snap.</li>'
 '<li><b>Inspector</b>: hover a parameter name for its specification description; the About section explains the selected widget; red values would not fit the binary form.</li>'
 '<li><b>Linking</b>: the Definition File ApplicationId must equal the user application applicationId; the user application inspector checks it and previews the layers in the window; the Definition File root lists who references it.</li>'
 '<li><b>Files</b>: Ctrl+S saves (.bin = binary Part 1-7 form, otherwise XML). Ctrl+Z / Ctrl+Y undo and redo. F1 opens help on the selection.</li></ol>')
for n, d in face:
    write(n + '.htm', n, '<h1>%s</h1><p>%s</p>' % (n, html.escape(d)))
for code in sorted(w):
    e = w[code]
    body = ['<h1>%s <small>(%s)</small></h1>' % (code, html.escape(e['name']))]
    if e['categories']:
        body.append('<p class="cat">Categories: %s</p>' % html.escape(', '.join(e['categories'])))
    body.append('<p>%s</p>' % html.escape(e['description']))
    if e['restriction']:
        body.append('<p><b>Restriction:</b> %s</p>' % html.escape(e['restriction']))
    body.append('<table><tr><th>Parameter</th><th>Change</th><th>Description</th></tr>')
    for n, c, d in e['params']:
        body.append('<tr><td>%s</td><td>%s</td><td>%s</td></tr>' % (html.escape(n), c, html.escape(d)))
    body.append('</table>')
    write(code + '.htm', code, '\n'.join(body))
# Table of contents (.hhc)
hhc = ['<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HTML><BODY><UL>',
       '<LI><OBJECT type="text/sitemap"><param name="Name" value="Getting started"><param name="Local" value="Getting-Started.htm"></OBJECT>',
       '<LI><OBJECT type="text/sitemap"><param name="Name" value="FACE display management"></OBJECT><UL>']
for n, d in face:
    hhc.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s"><param name="Local" value="%s.htm"></OBJECT>' % (n, n))
hhc.append('</UL><LI><OBJECT type="text/sitemap"><param name="Name" value="ARINC 661 widgets"></OBJECT><UL>')
for code in sorted(c for c in w if not c.endswith('_EXTENSION')):
    hhc.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s"><param name="Local" value="%s.htm"></OBJECT>' % (code, code))
hhc.append('</UL><LI><OBJECT type="text/sitemap"><param name="Name" value="ARINC 661 extensions"></OBJECT><UL>')
for code in sorted(c for c in w if c.endswith('_EXTENSION')):
    hhc.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s"><param name="Local" value="%s.htm"></OBJECT>' % (code, code))
hhc.append('</UL></UL></BODY></HTML>')
open(os.path.join(root, 'arinc661.hhc'), 'w', encoding='utf-8').write('\n'.join(hhc))
# Index (.hhk): every FACE and widget topic, plus every parameter name linked to up to 40 widgets
hhk = ['<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><HTML><BODY><UL>']
for n, d in face:
    hhk.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s"><param name="Local" value="%s.htm"></OBJECT>' % (n, n))
params = {}
for code in sorted(w):
    hhk.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s"><param name="Local" value="%s.htm"></OBJECT>' % (code, code))
    for n, c, d in w[code]['params']:
        params.setdefault(n, []).append(code)
for n in sorted(params):
    hhk.append('<LI><OBJECT type="text/sitemap"><param name="Name" value="%s">' % n + ''.join('<param name="Name" value="%s"><param name="Local" value="%s.htm">' % (c, c) for c in params[n][:40]) + '</OBJECT>')
hhk.append('</UL></BODY></HTML>')
open(os.path.join(root, 'arinc661.hhk'), 'w', encoding='utf-8').write('\n'.join(hhk))
hhp = ['[OPTIONS]', 'Compatibility=1.1 or later', 'Compiled file=..\\arinc661.chm', 'Contents file=arinc661.hhc', 'Index file=arinc661.hhk',
       'Default topic=Getting-Started.htm', 'Display compile progress=No', 'Full-text search=Yes', 'Language=0x409 English (United States)',
       'Title=ARINC 661 Editor Help', 'Default Window=main', '', '[WINDOWS]',
       'main="ARINC 661 Editor Help","arinc661.hhc","arinc661.hhk","Getting-Started.htm","Getting-Started.htm",,,,,0x63520,,0x384e,,,,,,,,0', '', '[FILES]']
hhp += files
open(os.path.join(root, 'arinc661.hhp'), 'w', encoding='utf-8').write('\n'.join(hhp) + '\n')
hhc_exe = r'C:\Program Files (x86)\HTML Help Workshop\hhc.exe'
r = subprocess.run([hhc_exe, os.path.join(root, 'arinc661.hhp')], capture_output=True, text=True)
print(r.stdout[-600:], r.stderr[-300:])
print('chm exists:', os.path.exists(r'C:\dev\xml\docs\arinc661.chm'))
