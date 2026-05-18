import os
import glob

authors_header = """/**
 * @file {filename}
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief 
 */
"""

for root, _, files in os.walk('src'):
    for file in files:
        if file.endswith('.cpp') or file.endswith('.h'):
            filepath = os.path.join(root, file)
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Check if header already exists
            if "@author Luk" in content:
                continue

            # Remove first line if it's just // filename
            lines = content.split('\n')
            if lines and lines[0].startswith('//') and file in lines[0]:
                content = '\n'.join(lines[1:]).lstrip()
            
            header = authors_header.format(filename=file)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(header + content)
