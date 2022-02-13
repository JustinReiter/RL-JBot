import csv, os

def load_file(file_name):
    with open(file_name, newline='') as fs:
        return list(csv.reader(fs))

def load_all_files(file_names):
    data = []
    for file_name in file_names:
        data.extend(load_file(file_name))

def get_log_file_names():
    files = []
    for dirpath, _, fileNames in os.walk(os.path.dirname(os.path.realpath(__file__)) + "/../../training/game_data"):
        for file in fileNames:
            files.append(os.path.abspath(os.path.join(dirpath, file)))
    return files
