import os, re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Find closing braces at the start of a line (end of functions/structs)
    # and ensure they are followed by exactly 3 newlines (2 blank lines).
    content = re.sub(r'(?m)^}(\s*)\n+', r'}\1\n\n\n', content)
    content = re.sub(r'(?m)^};(\s*)\n+', r'};\1\n\n\n', content)
    
    with open(filepath, 'w') as f:
        f.write(content)

# Generated image, animation & font data is skipped as it's overwritten whenever it is regenerated
def is_generated(filepath):
    parts = filepath.split(os.sep)
    return 'assets' in parts or filepath.endswith(os.path.join('widgets', 'art.c'))

# We want to process boards directory from tools script
base_dirs = ['../boards']

for bdir in base_dirs:
    full_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), bdir)
    if os.path.exists(full_dir):
        for root, _, files in os.walk(full_dir):
            for file in files:
                path = os.path.join(root, file)
                if (file.endswith('.c') or file.endswith('.h')) and not is_generated(path):
                    process_file(path)

print("Added 2 blank lines after top-level functions and structs!")
