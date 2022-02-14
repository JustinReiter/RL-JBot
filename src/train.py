from nn_model import NN_Model
from util.load_csvs import load_all_files, get_log_file_names
import numpy as np
model = NN_Model()

# Get the data used for training
training_files = get_log_file_names()

print(training_files[0])
print(training_files[1])

training_data = load_all_files(training_files[0]), load_all_files(training_files[1])

train_input = np.array([np.array(x) for x in training_data[0]])
train_output = np.array([np.array(y) for y in training_data[1]])

# train model
model.train(train_input, train_output)

# save model
model.save_model()
