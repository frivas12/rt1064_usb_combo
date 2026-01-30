# Usage: python3 new_config.py <struct-id>
import shutil
import sys
import os


do_copy = len(sys.argv) == 3 and sys.argv[2] == '--copy'
if len(sys.argv) > 3 or len(sys.argv) < 2:
    print(
        'usage: python3 new_config.py <struct-id> [--copy]',
        file=sys.stderr)
    exit(1)


struct_id = int(sys.argv[1], 0)
config_id: int | None = None

template_ids = {}
for x in os.listdir('./config/templates/'):
    template_ids[int(os.path.splitext(x)[0])] = f'./config/templates/{x}'

template_file = struct_id in template_ids and template_ids[struct_id] or None

if template_file is None:
    print(f'error (2): no template file for struct-type ID "{struct_id}"',
          file=sys.stderr)
    exit(2)

prefix_key = f'{struct_id:d}-'
existing = [x for x in os.listdir(
    './config') if x.startswith(prefix_key)]

if len(existing) == 0:
    config_id = 0
else:
    config_id = 1 + \
        max(map(lambda x: int(os.path.splitext(
            x[len(prefix_key):])[0]), existing))

output = f'./config/{prefix_key}{config_id:d}.xml'
if do_copy:
    shutil.copy(template_file, output)
print(output)
