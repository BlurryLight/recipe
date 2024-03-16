import argparse
import subprocess
from pathlib import Path

def remove_bytes(file_path):
    with open(file_path, 'rb') as file:
        data = file.read()
    with open(file_path, 'wb') as file:
        file.write(data[9:])

def merge_files(file1_path, file2_path, output_path):
    try:
        remove_bytes(file1_path)
        remove_bytes(file2_path)

        ffmpeg_cmd = f'ffmpeg -i "{file1_path}" -i "{file2_path}" -codec copy {output_path}'

        subprocess.run(ffmpeg_cmd, shell=True, check=True)

        print(f"Merged files successfully into '{output_path}'")
    except Exception as e:
        print(f"Error occurred: {str(e)}")

if __name__ == '__main__':
    files = list(Path('.').glob("*.m4s"))
    print(files)
    if(len(files) == 2):
        merge_files(files[0], files[1], "merge_output.mp4")
