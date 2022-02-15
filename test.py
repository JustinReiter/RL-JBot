from src.util.load_csvs import load_file
from datetime import datetime

FILE_NAMES = [
    "C:/Users/Justi/Desktop/projects/JBot/training/game_data/2022-02-13_19-52-54_output.csv",
    "C:/Users/Justi/Desktop/projects/JBot/training/game_data/2022-02-13_19-52-54_input.csv",

    "C:/Users/Justi/Desktop/projects/JBot/training/game_data/2022-02-13_21-04-05_output.csv",
    "C:/Users/Justi/Desktop/projects/JBot/training/game_data/2022-02-13_21-04-05_input.csv"
]

for file in FILE_NAMES:
    data = load_file(file)

    print("File: {}".format(file))
    print("\tNumber of lines: {}".format(len(data)))
    print("\tNumber of params: {}".format(len(data[0])))

    for line in data:
        if len(line) != len(data[0]):
            print("\t\tFound line with incorrect number of params: {}".format(len(line)))
            print("\t\t" + ",".join(line))
