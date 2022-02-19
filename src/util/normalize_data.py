from load_csvs import get_log_file_names, load_all_files, get_normalized_log_file_names
import math

class Vec3:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

MAX_REGION = Vec3(4096, 5120 + 880, 2044)
MAX_VELOCITY = Vec3(6000, 6000, 6000)
MAX_ANG_VELOCITY = Vec3(math.pi/2, math.pi, math.pi)

# grab lists of file names
input_files, output_files = get_log_file_names()
input_normalized, output_normalized = get_normalized_log_file_names()

# Fix directory of files to maintain same path between normalized and raw values
input_normalized = list(
    map(
        lambda x: x.replace("normalized_game_data", "game_data"),
        input_normalized
    )
)
output_normalized = list(
    map(
        lambda x: x.replace("normalized_game_data", "game_data"),
        output_normalized
    )
)

# find files that need to be normalized
input_to_convert = list(set(input_files) - set(input_normalized))
output_to_convert = list(set(output_files) - set(output_normalized))

print(input_to_convert)
print(output_to_convert)

# Inputs needs to be normalized
for ifname in input_to_convert:
    print("Converting the following file:\n\t{}".format(ifname))
    with open(ifname, "r") as rfstream, open(ifname.replace("game_data", "normalized_game_data"), "w") as wfstream:
        for row in rfstream.readlines():
            values = row.split(",")
            
            # Player values
            values[0] = str(float(values[0]) / MAX_REGION.x)
            values[1] = str(float(values[1]) / MAX_REGION.y)
            values[2] = str(float(values[2]) / MAX_REGION.z)

            values[3] = str(float(values[3]) / MAX_VELOCITY.x)
            values[4] = str(float(values[4]) / MAX_VELOCITY.y)
            values[5] = str(float(values[5]) / MAX_VELOCITY.z)

            values[6] = str(float(values[6]) / MAX_ANG_VELOCITY.x)
            values[7] = str(float(values[7]) / MAX_ANG_VELOCITY.y)
            values[8] = str(float(values[8]) / MAX_ANG_VELOCITY.z)

            # Opp values
            values[12] = str(float(values[12]) / MAX_REGION.x)
            values[13] = str(float(values[13]) / MAX_REGION.y)
            values[14] = str(float(values[14]) / MAX_REGION.z)

            values[15] = str(float(values[15]) / MAX_VELOCITY.x)
            values[16] = str(float(values[16]) / MAX_VELOCITY.y)
            values[17] = str(float(values[17]) / MAX_VELOCITY.z)

            values[18] = str(float(values[18]) / MAX_ANG_VELOCITY.x)
            values[19] = str(float(values[19]) / MAX_ANG_VELOCITY.y)
            values[20] = str(float(values[20]) / MAX_ANG_VELOCITY.z)

            # Ball values
            values[24] = str(float(values[24]) / MAX_REGION.x)
            values[25] = str(float(values[25]) / MAX_REGION.y)
            values[26] = str(float(values[26]) / MAX_REGION.z)

            values[27] = str(float(values[27]) / MAX_VELOCITY.x)
            values[28] = str(float(values[28]) / MAX_VELOCITY.y)
            values[29] = str(float(values[29]) / MAX_VELOCITY.z)

            values[30] = str(float(values[30]) / MAX_ANG_VELOCITY.x)
            values[31] = str(float(values[31]) / MAX_ANG_VELOCITY.y)
            values[32] = str(float(values[32]) / MAX_ANG_VELOCITY.z)

            wfstream.write("{}".format(",".join(values)))

# Outputs are automatically normalized
for ofname in output_to_convert:
    print("Converting the following file:\n\t{}".format(ofname))
    with open(ofname, "r") as rfstream, open(ofname.replace("game_data", "normalized_game_data"), "w") as wfstream:
        for row in rfstream.readlines():
            wfstream.write("{}".format(row))
