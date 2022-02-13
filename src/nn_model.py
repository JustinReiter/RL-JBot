import numpy as np
from tensorflow import keras

class NN_Model:

    def __init__(self):
        self.model = keras.Sequential(
            [
                keras.layers.Dense(64, input_dim=38, activation='relu'),
                keras.layers.Dense(56, activation='relu'),
                keras.layers.Dense(48, activation='relu'),
                keras.layers.Dense(32, activation='relu'),
                keras.layers.Dense(8, activation='sigmoid')
            ]
        )
        
        self.model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])

    def predict(self, input: np.darray):
        pass

    # Similar to self.predict() with adjustment to weights & bias
    def train(self, input: np.darray):
        pass
