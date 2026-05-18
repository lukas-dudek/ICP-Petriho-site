import sys
import re

def de_ai_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Odstranění doxygen hlaviček souboru typu:
    # /**
    #  * @file    ...
    #  * @brief   ...
    #  ...
    #  */
    content = re.sub(r'/\*\*\s*\n\s*\* @file.*?\*/\s*\n', '// ' + filepath.split('/')[-1] + '\n\n', content, flags=re.DOTALL)
    
    # Odstranění dalších dokonalých doxygen komentářů tříd a funkcí (pokud jsou jednoduché):
    # Nahradíme /** ... */ za obyčejné //
    def replace_doxygen(match):
        text = match.group(0)
        lines = text.split('\n')
        out = []
        for line in lines:
            if line.strip() == '/**' or line.strip() == '*/':
                continue
            line = re.sub(r'^\s*\*\s?', '// ', line)
            out.append(line)
        return '\n'.join(out)

    content = re.sub(r'/\*\*.*?\*/', replace_doxygen, content, flags=re.DOTALL)

    # Odstranění blokových separátorů
    content = re.sub(r'// ======================================================================\n', '', content)
    content = re.sub(r'// ====================.*?====================\n', '', content)
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == "__main__":
    for arg in sys.argv[1:]:
        de_ai_file(arg)
