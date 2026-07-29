import os
import sys

def convert_to_utf8(file_path):
    """
    Attempts to read a file and convert it to UTF-8 if it's not already.
    Primarily targets Shift-JIS (cp932) which is common in Windows-generated files.
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            f.read()
        return False, "Already UTF-8 or compatible"
    except UnicodeDecodeError:
        pass

    # Common encodings in Japanese environments
    encodings_to_try = ['cp932', 'shift_jis', 'euc_jp', 'iso-2022-jp', 'latin-1']
    
    for enc in encodings_to_try:
        try:
            with open(file_path, 'r', encoding=enc) as f:
                content = f.read()
            
            # Write back as UTF-8
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            
            return True, f"Converted from {enc} to UTF-8"
        except UnicodeDecodeError:
            continue
        except Exception as e:
            return False, f"Error reading as {enc}: {e}"
            
    return False, "Failed to decode with known encodings"

def main():
    target_dir = sys.argv[1] if len(sys.argv) > 1 else "."
    target_extensions = {'.py', '.urdf', '.xacro', '.xml', '.yaml', '.yml', '.cfg', '.txt', '.md'}
    
    target_dir = os.path.abspath(target_dir)
    print(f"Scanning directory for non-UTF-8 files: {target_dir}")
    
    converted_count = 0
    for root, dirs, files in os.walk(target_dir):
        for file in files:
            ext = os.path.splitext(file)[1].lower()
            if ext in target_extensions:
                file_path = os.path.join(root, file)
                converted, msg = convert_to_utf8(file_path)
                if converted:
                    print(f"[CONVERTED] {file_path}: {msg}")
                    converted_count += 1

    print(f"Done! Converted {converted_count} files to UTF-8.")

if __name__ == '__main__':
    main()
