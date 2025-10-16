#!/usr/bin/env python3
"""
Generate a Graphviz dot representation of the JAPAN-MOLLER Parity Analysis database schema.

This script parses the qwparity_schema.sql file and creates a visual diagram showing:
- All tables as nodes
- Foreign key relationships as edges
- Primary keys and important fields highlighted
- Different node styles for different table types

Usage:
    python3 generate_schema_diagram.py > schema_diagram.dot
    dot -Tpng schema_diagram.dot -o schema_diagram.png
    dot -Tsvg schema_diagram.dot -o schema_diagram.svg
"""

import re
import sys
from pathlib import Path

def parse_schema_file(filename):
    """Parse the SQL schema file and extract table definitions and relationships."""
    
    tables = {}
    relationships = []
    
    with open(filename, 'r') as f:
        content = f.read()
    
    # Remove comments and normalize whitespace
    content = re.sub(r'--.*$', '', content, flags=re.MULTILINE)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    
    # Extract CREATE TABLE statements
    table_pattern = r'CREATE TABLE (\w+)\s*\((.*?)\);'
    
    for match in re.finditer(table_pattern, content, re.DOTALL | re.IGNORECASE):
        table_name = match.group(1)
        table_def = match.group(2)
        
        # Parse columns
        columns = []
        primary_key = None
        
        # Split by comma but respect parentheses
        column_lines = []
        paren_count = 0
        current_line = ""
        
        for char in table_def:
            if char == '(':
                paren_count += 1
            elif char == ')':
                paren_count -= 1
            elif char == ',' and paren_count == 0:
                column_lines.append(current_line.strip())
                current_line = ""
                continue
            current_line += char
        
        if current_line.strip():
            column_lines.append(current_line.strip())
        
        for line in column_lines:
            line = line.strip()
            if not line:
                continue
                
            # Skip constraints for now, focus on column definitions
            if line.upper().startswith(('PRIMARY KEY', 'FOREIGN KEY', 'KEY', 'INDEX', 'CONSTRAINT')):
                continue
                
            # Extract column name and type
            parts = line.split()
            if len(parts) >= 2:
                col_name = parts[0]
                col_type = parts[1]
                
                is_pk = 'PRIMARY KEY' in line.upper() or 'AUTO_INCREMENT' in line.upper()
                is_fk = False
                
                # Look for foreign key patterns in column name
                if col_name.endswith('_id') and col_name != f"{table_name}_id":
                    is_fk = True
                    # Determine referenced table
                    ref_table = col_name.replace('_id', '')
                    # Handle special cases
                    if ref_table == 'main_detector':
                        ref_table = 'main_detector'
                    elif ref_table == 'lumi_detector':
                        ref_table = 'lumi_detector'
                    elif ref_table == 'sc_detector':
                        ref_table = 'sc_detector'
                    elif ref_table == 'measurement_type':
                        ref_table = 'measurement_type'
                    elif ref_table == 'error_code':
                        ref_table = 'error_code'
                    elif ref_table == 'slope_type':
                        ref_table = 'slope_type'
                    elif ref_table == 'modulation_type':
                        ref_table = 'modulation_type'
                    
                    relationships.append((table_name, ref_table, col_name))
                
                if is_pk:
                    primary_key = col_name
                
                columns.append({
                    'name': col_name,
                    'type': col_type,
                    'is_pk': is_pk,
                    'is_fk': is_fk
                })
        
        tables[table_name] = {
            'columns': columns,
            'primary_key': primary_key
        }
    
    return tables, relationships

def categorize_tables(tables):
    """Categorize tables by their purpose for better visualization."""
    categories = {
        'core': ['run', 'runlet', 'analysis'],
        'reference': ['good_for', 'run_quality', 'measurement_type', 'slope_type', 'monitor', 
                     'main_detector', 'lumi_detector', 'error_code', 'modulation_type', 'sc_detector'],
        'data': ['md_data', 'lumi_data', 'beam', 'md_slope', 'lumi_slope'],
        'errors': ['general_errors', 'md_errors', 'lumi_errors', 'beam_errors'],
        'slow_controls': ['slow_controls_settings', 'slow_controls_data', 'slow_controls_strings'],
        'misc': ['seeds', 'parameter_files', 'bf_test', 'beam_optics', 'db_schema']
    }
    
    table_categories = {}
    for category, table_list in categories.items():
        for table in table_list:
            if table in tables:
                table_categories[table] = category
    
    # Default category for any unclassified tables
    for table in tables:
        if table not in table_categories:
            table_categories[table] = 'misc'
    
    return table_categories

def generate_dot_output(tables, relationships, table_categories):
    """Generate the Graphviz DOT representation."""
    
    # Node colors by role
    role_colors = {
        'core': '#4A90E2',         # Blue
        'reference': '#7ED321',    # Green
        'data': '#F5A623',         # Orange
        'errors': '#D0021B',       # Red
        'slow_controls': '#9013FE', # Purple
        'misc': '#50E3C2'          # Teal
    }
    
    print('digraph qwparity_schema {')
    print('    layout=neato;')
    print('    node [shape=box, fontname="Arial", fontsize=10];')
    print('    edge [fontname="Arial", fontsize=8];')
    print('    overlap=false;')
    print('    splines=true;')
    print('')
    
    # Generate all table nodes with role-based coloring
    print('    // Table nodes colored by role')
    for table_name, table in tables.items():
        category = table_categories.get(table_name, 'misc')
        color = role_colors.get(category, '#CCCCCC')
        
        # Build the table label with vertical field layout using \l for line breaks
        label_parts = [f"{table_name}\\l"]
        label_parts.append("\\l")  # Extra line break after table name
        
        for col in table['columns'][:8]:  # Limit to 8 columns for neato layout
            col_label = col['name']
            if col['is_pk']:
                col_label = f"🔑 {col_label}"
            elif col['is_fk']:
                col_label = f"🔗 {col_label}"
            
            # Add type information (shorter for neato)
            col_type = col['type'].upper()
            if len(col_type) > 15:
                col_type = col_type[:12] + "..."
            
            label_parts.append(f"{col_label} : {col_type}\\l")
        
        if len(table['columns']) > 8:
            label_parts.append("...\\l")
        
        # Join without separators since we're using \l for line breaks
        label = "".join(label_parts)
        
        print(f'    {table_name} [label="{label}", style=filled, fillcolor="{color}"];')
    
    print('')
    
    # Add relationships
    print('    // Relationships')
    for source_table, target_table, fk_column in relationships:
        if source_table in tables and target_table in tables:
            # Use different edge styles based on relationship type
            edge_style = 'solid'
            edge_color = 'black'
            
            if 'error' in source_table.lower():
                edge_color = 'red'
                edge_style = 'dashed'
            elif 'slope' in source_table.lower():
                edge_color = 'blue'
                edge_style = 'dotted'
            elif 'slow_controls' in source_table.lower():
                edge_color = 'purple'
                edge_style = 'dashed'
            
            print(f'    {source_table} -> {target_table} '
                  f'[color={edge_color}, style={edge_style}, '
                  f'label="{fk_column}", fontsize=7];')
    
    print('}')


def main():
    """Main function to generate the schema diagram."""

    import argparse

    default_schema_path = Path(__file__).parent.parent.parent / "Parity" / "prminput" / "qwparity_schema.sql"
    parser = argparse.ArgumentParser(description="Generate Graphviz DOT diagram from JAPAN-MOLLER schema.")
    parser.add_argument(
        "--schema",
        type=str,
        default=str(default_schema_path),
        help="Path to qwparity_schema.sql (default: %(default)s)"
    )
    args = parser.parse_args()
    schema_file = Path(args.schema)

    if not schema_file.exists():
        print(f"Error: Schema file not found at {schema_file}", file=sys.stderr)
        sys.exit(1)

    try:
        tables, relationships = parse_schema_file(schema_file)
        table_categories = categorize_tables(tables)
        generate_dot_output(tables, relationships, table_categories)

        # Print usage instructions to stderr
        print("", file=sys.stderr)
        print("Generated Graphviz DOT output. To create images:", file=sys.stderr)
        print("  dot -Tpng schema_diagram.dot -o schema_diagram.png", file=sys.stderr)
        print("  dot -Tsvg schema_diagram.dot -o schema_diagram.svg", file=sys.stderr)
        print("  dot -Tpdf schema_diagram.dot -o schema_diagram.pdf", file=sys.stderr)
        print("", file=sys.stderr)
        print(f"Found {len(tables)} tables and {len(relationships)} relationships.", file=sys.stderr)

    except Exception as e:
        print(f"Error processing schema file: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()