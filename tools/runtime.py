# Generates src/arinc661/arinc661_runtime.c from the ARINC 661 Part 1-7 text: every widget and
# extension type's Runtime Modifiable Parameters table, joined with Table 4.6-8, Parameter Types.
# Usage: pdftotext -raw ARINC_SPECIFICATION_661P1-7.pdf raw.txt, then, from the repository root,
#        python tools/runtime.py raw.txt src/arinc661/arinc661_runtime.c
# With --report in place of the output path, prints what was parsed and left out, and writes nothing.
import re
import sys

raw_path, output_path = sys.argv[1], sys.argv[2]
lines = open(raw_path, encoding='utf-8', errors='replace', newline='').read().replace('\r', '').split('\n')
lines = [l.replace('\f', '').strip() for l in lines]

# Type names the definition module knows, such as A661_ACTIVE_AREA
definition = open('src/arinc661/arinc661_definition.c', encoding='utf-8').read()
type_names = re.findall(r'^#define TYPE_(A661_\w+)\s+\("A661_\w+"\)', definition, re.M)
by_key = {t[len('A661_'):].replace('_', ''): t for t in type_names}
# Extension tables titled other than their type
by_key['LEGENDSTRINGALIGNMENT'] = 'A661_LEGEND_ALIGN_EXTENSION'
by_key['VISIBLECHILDINDEXEXTENSION'] = 'A661_CHILD_INDEX_EXTENSION'

# Parameter names of each type, from the generated help, to check the extracted names
help_text = open('examples/arinc661_help.c', encoding='utf-8').read()
help_params = re.findall(r'^    \{"(\w+)", "(?:D|R|DR)", ', help_text, re.M)
help_types = {}
for code, first, count in re.findall(r'^    \{"(A661_\w+)", .*, (\d+)U?, (\d+)U?\},$', help_text, re.M):
    help_types[code] = set(help_params[int(first):int(first) + int(count)])

# Table 4.6-8: ParameterIdent name, value, and the structure carrying it
idents = {}
start = next(i for i, l in enumerate(lines) if l.startswith('Table 4.6-8 ') and 'Parameter Types' in l and i > 20000)
for l in lines[start:start + 1200]:
    m = re.match(r'^(A661_\w+) (0x[0-9A-Fa-f]{4}) (A661_ParameterStructure_\w+)', l)
    if m:
        idents[m.group(1)] = (int(m.group(2), 16), m.group(3))
    if l.startswith('Table 4.6-9'):
        break

STRUCTURES = {
    'A661_ParameterStructure_1Byte': 'ARINC661_RUNTIME_1BYTE',
    'A661_ParameterStructure_2Bytes': 'ARINC661_RUNTIME_2BYTES',
    'A661_ParameterStructure_4Bytes': 'ARINC661_RUNTIME_4BYTES',
    'A661_ParameterStructure_8Bytes': 'ARINC661_RUNTIME_8BYTES',
    'A661_ParameterStructure_String': 'ARINC661_RUNTIME_STRING',
    'A661_ParameterStructure_StringArray': 'ARINC661_RUNTIME_STRING_ARRAY',
}

KINDS = {
    'uchar': 'ARINC661_PARAMETER_UCHAR',
    'ushort': 'ARINC661_PARAMETER_USHORT',
    'short': 'ARINC661_PARAMETER_SHORT',
    'long': 'ARINC661_PARAMETER_LONG',
    'integer': 'ARINC661_PARAMETER_LONG',
    'ulong': 'ARINC661_PARAMETER_ULONG',
    'float': 'ARINC661_PARAMETER_FLOAT',
    'fr(180)': 'ARINC661_PARAMETER_FR180',
    'fr(32768)': 'ARINC661_PARAMETER_FR32768',
    'string': 'ARINC661_PARAMETER_STRING',
}

TITLE = re.compile(r'^Table (\d+(?:\.\d+)+-\d+[a-z]?) .\s?([\w ]+?) Runtime Modifiable Parameters$')
HEADING = re.compile(r'^(\d+\.\d+\.\d+(\.\d+)*|\d+\.\d+) [A-Z]')
SKIP = re.compile(r'^(ARINC SPECIFICATION 661|\d+\.0 [A-Z ]+$|Name of the Parameter to Set|ParameterIdent used|the ParameterStructure$|Type of Structure|SUPPLEMENT)')
KIND = r'\{?[a-z]+(?:\(\d+\))?\}?\+?'
ROW = re.compile(r'^(?P<names>.+?) (?P<type>' + KIND + r'(?: ' + KIND + r')?(?: x \d+)?) (?P<size>\{?\d+\}?\+?(?: x \d+)?|N/A) (?P<ident>A661_\w+)$')

def CleanNames(names):
    # Drop column header pieces and notes that the text puts in front of a row's names
    names = re.sub(r'Size \(bits\)', ' ', names)
    names = re.sub(r'(?i)see section [\d.]+', ' ', names)
    if '. ' in names:
        names = names[names.rindex('. ') + 2:]
    names = names.replace('ParameterStructure', ' ')
    names = re.sub(r'(?i)ParameterIdent used in the', ' ', names)
    names = names.replace(' Long/scaled', ' ')
    names = re.sub(r'^\s*Type ', ' ', names)
    return re.sub(r'\s+', ' ', names).strip()


tables = []
unparsed = []
i = 0
while i < len(lines):
    m = TITLE.match(lines[i])
    if not m or i < 3000:
        i += 1
        continue
    title = m.group(2).replace(' ', '')
    if title.startswith('A661_'):
        title = title[len('A661_'):]
    type_name = by_key.get(title.upper().replace('_', ''))
    rows = []
    pending = []
    j = i + 1
    while j < len(lines):
        l = lines[j]
        if (not l) or SKIP.match(l):
            j += 1
            continue
        if TITLE.match(l) or l.startswith('Table ') or (HEADING.match(l) and not pending):
            break
        pending.append(l)
        if re.search(r'\bA661_\w+$', l) and not l.startswith('A661_ParameterStructure'):
            text = ' '.join(pending)
            pending = []
            text = re.sub(r'\[[^\]]*\]', '', text)
            # Spec typos such as "PosX,PosY longx 2 32x 2", and a capitalized "String" type
            text = re.sub(r',(?=\S)', ', ', text)
            text = re.sub(r'(\w)x (\d)', r'\1 x \2', text)
            text = re.sub(r' String (\{32\}\+) ', r' string \1 ', text)
            text = re.sub(r'\s+', ' ', text).strip()
            r = ROW.match(text)
            if r:
                rows.append((CleanNames(r.group('names')), r.group('type'), r.group('size'), r.group('ident')))
            else:
                unparsed.append((title, text))
        elif len(pending) > 6:
            # Prose after the table
            break
        j += 1
    tables.append((title, type_name, rows))
    i = j

# Build the C rows, keeping only those the decoder can turn into property text
types_out = []
rows_out = []
left_out = []
name_warnings = []
for title, type_name, rows in tables:
    if type_name is None:
        left_out.append((title, 'no matching type'))
        continue
    first = len(rows_out)
    for names, value_type, size, ident in rows:
        if ident not in idents:
            # A name cut short in the table, such as A661_MAP_SYNCHRONIZATION_NU
            matches = [k for k in idents if k.startswith(ident)]
            if len(matches) == 1:
                ident = matches[0]
        if ident not in idents:
            left_out.append((title, ident + ' is not in Table 4.6-8'))
            continue
        value, structure = idents[ident]
        if structure not in STRUCTURES:
            left_out.append((title, ident + ' uses ' + structure))
            continue
        elements = value_type.strip('{}+').replace(' x 2', '').split(' ')
        is_pair = size.endswith(' x 2')
        if is_pair and (1 == len(elements)):
            elements = elements * 2
        if any(e not in KINDS for e in elements) or (len(elements) != (2 if is_pair else 1)):
            left_out.append((title, ident + ' has type ' + value_type))
            continue
        if is_pair and (', ' not in names) and (2 == len(names.split(' '))):
            # A pair printed without its comma, such as "MinValue MaxValue"
            names = names.replace(' ', ', ')
        parts = [re.sub(r'\s+', '', p) for p in names.split(', ')]
        if len(parts) != len(elements):
            left_out.append((title, ident + ' names ' + names))
            continue
        known = help_types.get(type_name, set())
        for p in parts:
            if known and p not in known:
                name_warnings.append((type_name, ident, p))
        rows_out.append((value, ident, STRUCTURES[structure], KINDS[elements[0]], parts[0], KINDS[elements[1]] if is_pair else 'ARINC661_PARAMETER_UCHAR', parts[1] if is_pair else None))
    types_out.append((type_name, first, len(rows_out) - first))

if output_path == '--report':
    print('ParameterIdents in Table 4.6-8:', len(idents))
    print('Types:', len(types_out), 'rows kept:', len(rows_out))
    print('Types without a table:', sorted(set(type_names) - {t[0] for t in types_out}))
    for entry in left_out:
        print('left out', entry)
    for entry in unparsed:
        print('unparsed', entry)
    for entry in name_warnings:
        print('name not among the type parameters', entry)
    sys.exit(0)

types_out.sort(key=lambda t: t[0])


def c_string(text):
    return 'NULL' if text is None else '"' + text + '"'


row_cells = [('{0x%04X,' % r[0], r[2] + ',', r[3] + ',', c_string(r[4]) + ',', r[5] + ',', c_string(r[6]) + '},', '// ' + r[1]) for r in rows_out]
widths = [max(len(c[k]) for c in row_cells) for k in range(6)]
row_lines = ['    ' + ' '.join(c[k].ljust(widths[k]) for k in range(6)) + ' ' + c[6] for c in row_cells]
type_cells = [('{"' + t[0] + '",', '%d,' % t[1], '%d},' % t[2]) for t in types_out]
type_widths = [max(len(c[k]) for c in type_cells) for k in range(3)]
type_lines = ['    ' + ' '.join(c[k].ljust(type_widths[k]) for k in range(3)).rstrip() for c in type_cells]

out = '''/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 3 and 8: the Runtime Modifiable Parameters table of every widget
 * and extension, and Table 4.6-8, Parameter Types. Generated from the specification text by
 * tools/runtime.py. Rows carried by a structure Arinc661_RunTimeStructure_t does not list, such
 * as arrays and buffers, and rows whose ParameterIdent Table 4.6-8 does not list, are left out.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_runtime.h"
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

/** One row of a Runtime Modifiable Parameters table. */
typedef struct RunTimeRow_s
{
    uint32_t                    ident;      ///< ParameterIdent
    Arinc661_RunTimeStructure_t structure;  ///< Structure carrying the value
    Arinc661_ParameterKind_t    kind;       ///< Type of the value, or of the first value of a pair
    const char_t*               name;       ///< Parameter set
    Arinc661_ParameterKind_t    secondKind; ///< Type of the second value of a pair; unused otherwise
    const char_t*               secondName; ///< Second parameter set by a pair, or NULL
} RunTimeRow_t;

/** One widget or extension type's table. */
typedef struct RunTimeType_s
{
    const char_t* type;     ///< A661_ type name
    uint32_t      firstRow; ///< Index of the type's first row in c_runTimeRows
    uint32_t      rowCount; ///< Number of the type's rows
} RunTimeType_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/** Rows of every type, grouped by type, each group in table order. */
static const RunTimeRow_t c_runTimeRows[] =
{
''' + '\n'.join(row_lines) + '''
};

/** Types in name order, each with its group of rows. */
static const RunTimeType_t c_runTimeTypes[] =
{
''' + '\n'.join(type_lines) + '''
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \\par Details:
 *  Finds the type's table, then the row of the ParameterIdent in it. Compares at most
 *  ARINC661_MAX_NAME_BYTES characters of the type name.
 */
bool_t Arinc661_RunTime_GetParameter(const char_t* typeName, uint32_t parameterIdent, Arinc661_RunTimeParameter_t* parameter)
{
    const RunTimeType_t* type;
    const RunTimeRow_t*  row;
    uint32_t             index;
    bool_t               names_match;
    bool_t               is_found;

    LOGASSERT(NULL != typeName);
    LOGASSERT(NULL != parameter);

    type = NULL;
    for (index = 0; index < COUNT_OF(c_runTimeTypes); ++index)
    {
        names_match = (0 == strncmp(typeName, c_runTimeTypes[index].type, ARINC661_MAX_NAME_BYTES));
        if (names_match)
        {
            type = &c_runTimeTypes[index];
            break;
        }
    }

    row = NULL;
    if (NULL != type)
    {
        for (index = type->firstRow; index < (type->firstRow + type->rowCount); ++index)
        {
            if (parameterIdent == c_runTimeRows[index].ident)
            {
                row = &c_runTimeRows[index];
                break;
            }
        }
    }

    is_found = (NULL != row);
    if (is_found)
    {
        parameter->ident      = row->ident;
        parameter->structure  = row->structure;
        parameter->kind       = row->kind;
        parameter->name       = row->name;
        parameter->secondKind = row->secondKind;
        parameter->secondName = row->secondName;
    }

    return is_found;
}
'''
with open(output_path, 'wb') as f:
    f.write(out.replace('\n', '\r\n').encode('utf-8'))
print('wrote', output_path, len(types_out), 'types', len(rows_out), 'rows')
