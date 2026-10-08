# Generates examples/arinc661_help.c and help.json from the ARINC 661 Part 1-7 text.
# Usage: pdftotext -raw ARINC_SPECIFICATION_661P1-7.pdf raw.txt ; python tools/help.py   (run in the folder holding raw.txt)
import re, json, sys
raw = open('raw.txt', encoding='utf-8', errors='replace', newline='').read().replace(chr(13), '').split(chr(10))

# widget library body: from first "3.3.1 " heading in body to the Supplement change notes
start = next(i for i, l in enumerate(raw) if re.match(r'^3\.3\.1 ActiveArea$', l) and i > 3000)
end = next(i for i, l in enumerate(raw) if re.match(r'^4\.0 ', l) and i > start) if any(re.match(r'^4\.0 ', l) for l in raw[start:]) else len(raw)
# also extension sections (8.x)
body = raw[start:]
heading_re = re.compile(r'^(3\.\d+\.\d+(\.\d+)?|8\.\d+\.\d+) ([A-Z][A-Za-z0-9_]+)$')
skip_re = re.compile(r'^(ARINC SPECIFICATION|SUPPLEMENT|\d+\.0 WIDGET (LIBRARY|EXTENSIONS)|Parameters Change Description|Commonly used parameters|Specific [Pp]arameters.*|Parameters$|Change Description$|Figure .*|Possible Widgets for Extension:.*|See Table 8\.4\.)')
param_re = re.compile(r'^([A-Z][A-Za-z0-9]*(?: [A-Z][A-Za-z0-9]*)?) (D|R|DR) (.*)$')

def clean(t):
    t = t.replace('\ufffd', '-').replace('\u2013', '-').replace('\u2014', '-')
    t = re.sub(r'[^\x20-\x7e]', '-', t)
    return re.sub(r'\s+', ' ', t).strip()

widgets = {}
i = 0
n = len(body)
seen_tables = set()
while i < n:
    m = heading_re.match(body[i])
    if not m:
        i += 1
        continue
    name = m.group(3)
    j = i + 1
    categories, description, restriction = [], [], []
    mode = None
    params = []
    code = None
    # scan until Creation Structure table or next heading
    while j < n and not heading_re.match(body[j]):
        l = body[j].replace(chr(12), '').strip()
        if re.search(r'Creation Structure is defined in Table', l) or re.match(r'^Table .* Creation Structure', l):
            break
        if l.startswith('Categories:'):
            mode = 'cat'
        elif l.startswith('Description:') or l == 'Description' or l.endswith('Description:'):
            if l.endswith('Description:') and not l.startswith('Description'):
                categories.append(l[:-len('Description:')].strip())
            mode = 'desc'
            rest = l.split('Description:', 1)[1].strip() if 'Description:' in l else ''
            if rest:
                description.append(rest)
        elif l.startswith('Restriction:') or l.startswith('Restrictions:'):
            mode = 'restr'
            rest = l.split(':', 1)[1].strip()
            if rest and rest != 'None':
                restriction.append(rest)
        elif re.search(r'Parameters are defined in Table', l):
            mode = 'pre'
        elif re.match(r'^Table [\d.\-]+ ?.{0,2}\s*[A-Za-z0-9_ ]*Parameters$', l):
            mode = 'params'
        elif skip_re.match(l) or l == '':
            pass
        elif mode == 'cat':
            if l.startswith('\ufffd') or l.startswith('-') or l.startswith('•'):
                categories.append(clean(l.lstrip('\ufffd-• ')))
            elif l == 'None':
                mode = None
            else:
                mode = 'desc'
                description.append(l)
        elif mode == 'desc':
            description.append(l)
        elif mode == 'restr':
            if l != 'None':
                restriction.append(l)
        elif mode == 'params':
            pm = param_re.match(l)
            if pm:
                pname = pm.group(1).replace(' ', '')
                if pname in ('WidgetType', 'ExtensionType'):
                    code = pm.group(3).strip().replace('A661 ', 'A661_')
                params.append([pname, pm.group(2), pm.group(3)])
            elif params:
                params[-1][2] += ' ' + l
        j += 1
    if code and code.startswith('A661_') and code not in widgets:
        widgets[code] = {
            'name': name,
            'categories': [clean(c) for c in categories if c],
            'description': clean(' '.join(description)),
            'restriction': clean(' '.join(restriction)),
            'params': [(p[0], p[1], clean(p[2])) for p in params if p[0] not in ('WidgetType', 'ExtensionType')],
        }
    i = j if j > i else i + 1

# extensions: section 8 tables use "Extension Parameters" naming; try to capture by A661_*_EXTENSION codes
print(len(widgets), file=sys.stderr)
json.dump(widgets, open('help.json', 'w'), indent=1)

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

out = []
out.append('''/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 3.3 to 3.10 and 8: the Categories, Description, Restriction, and
 * Parameters table of every widget and extension. Generated from the specification text.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_help.h"

#include "faceoe_utl/faceoe_utl_defines.h"

#include "os/os_assert.h"

#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** One row of a Parameters table. */
typedef struct HelpParameter_s
{
    const char_t* name;        ///< Parameter name, without spaces
    const char_t* change;      ///< D, R, or DR: definition time, run time, or both
    const char_t* description; ///< Description column
} HelpParameter_t;

/** One widget or extension section. */
typedef struct HelpWidget_s
{
    const char_t* code;           ///< A661_ type name
    const char_t* categories;     ///< Categories, comma separated
    const char_t* description;    ///< Description paragraph
    const char_t* restriction;    ///< Restriction paragraph, or empty
    uint32_t      firstParameter; ///< Index into c_helpParameters
    uint32_t      parameterCount; ///< Rows
} HelpWidget_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

static const HelpParameter_t c_helpParameters[] =
{''')
widget_rows = []
count = 0
for code in sorted(widgets):
    w = widgets[code]
    first = count
    out.append('    /* %s */' % code)
    for pname, change, desc in w['params']:
        out.append('    {%s, %s, %s},' % (cstr(pname), cstr(change), cstr(desc)))
        count += 1
    widget_rows.append((code, w, first, count - first))
out.append('    {"", "", ""}\n};\n')
out.append('static const HelpWidget_t c_helpWidgets[] =\n{')
for code, w, first, cnt in widget_rows:
    out.append('    {%s, %s, %s, %s, %uU, %uU},' % (cstr(code), cstr(', '.join(w['categories'])), cstr(w['description']), cstr(w['restriction']), first, cnt))
out.append('''};

#define HELP_WIDGET_COUNT ((uint32_t)COUNT_OF(c_helpWidgets))

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

static const HelpWidget_t* FindHelp(const char_t* type);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \\brief Finds a widget's help section by type name.
 */
static const HelpWidget_t* FindHelp(const char_t* type)
{
    const HelpWidget_t* help;
    uint32_t            index;

    help = NULL;
    for (index = 0; index < HELP_WIDGET_COUNT; ++index)
    {
        if (0 == strcmp(type, c_helpWidgets[index].code))
        {
            help = &c_helpWidgets[index];
            break;
        }
    }

    return help;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \\par Details:
 *  Empty strings stand for sections the specification does not give.
 */
bool_t Arinc661_Help_GetType(const char_t* type, const char_t** description, const char_t** categories, const char_t** restriction)
{
    const HelpWidget_t* help;

    LOGASSERT(NULL != type);
    LOGASSERT(NULL != description);
    LOGASSERT(NULL != categories);
    LOGASSERT(NULL != restriction);
    (*description) = "";
    (*categories)  = "";
    (*restriction) = "";
    help = FindHelp(type);
    if (NULL != help)
    {
        (*description) = help->description;
        (*categories)  = help->categories;
        (*restriction) = help->restriction;
    }

    return !!(NULL != help);
}

/*************************************************************************************************/
/**
 * \\par Details:
 *  Looks the parameter up in the widget's own table first, then in any table, since the
 *  common parameters of section 3.1.3 are described once per widget.
 */
const char_t* Arinc661_Help_GetParameter(const char_t* type, const char_t* parameterName, const char_t** change)
{
    const HelpWidget_t* help;
    const char_t*       description;
    uint32_t            index;

    LOGASSERT(NULL != type);
    LOGASSERT(NULL != parameterName);
    description = "";
    if (NULL != change)
    {
        (*change) = "";
    }
    help = FindHelp(type);
    if (NULL != help)
    {
        for (index = help->firstParameter; index < (help->firstParameter + help->parameterCount); ++index)
        {
            if (0 == strcmp(parameterName, c_helpParameters[index].name))
            {
                description = c_helpParameters[index].description;
                if (NULL != change)
                {
                    (*change) = c_helpParameters[index].change;
                }
                break;
            }
        }
    }
    if ('\\0' == description[0])
    {
        for (index = 0; '\\0' != c_helpParameters[index].name[0]; ++index)
        {
            if (0 == strcmp(parameterName, c_helpParameters[index].name))
            {
                description = c_helpParameters[index].description;
                if (NULL != change)
                {
                    (*change) = c_helpParameters[index].change;
                }
                break;
            }
        }
    }

    return description;
}
''')
open(r'C:\dev\xml\examples\arinc661_help.c', 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
print('widgets', len(widgets), 'params', count, file=sys.stderr)
missing = [c for c in widgets if not widgets[c]['description']]
print('no description:', missing, file=sys.stderr)
