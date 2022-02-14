import os
import pandas as pd

# loads a single CSV into a list
def load_file(file_name):
    return pd.read_csv(file_name).values.tolist()

# loads all CSVs into a single list
def load_all_files(file_names):
    data = []
    for file_name in file_names:
        data.extend(load_file(file_name))
    
    return data

# returns a tuple of input/output file paths sorted in same order
def get_log_file_names():

    files = []
    for dirpath, _, fileNames in os.walk(os.path.dirname(os.path.realpath(__file__)) + "/../../training/game_data/"):
        for file in fileNames:
            files.append(os.path.abspath(os.path.join(dirpath, file)))

    input_files, output_files = [], []
    for f in files:
        if 'input' in f:
            input_files.append(f)
        else:
            output_files.append(f)
    
    input_files.sort()
    output_files.sort()

    return input_files, output_files
