import numpy as np
import os
from datetime import datetime
from tensorflow import keras, expand_dims

class NN_Model:

    def __init__(self):
        self.model = keras.Sequential(
            [
                keras.layers.Dense(128, input_dim=67, activation='relu'),
                keras.layers.Dense(128, activation='relu'),
                keras.layers.Dense(128, activation='relu'),
                keras.layers.Dense(128, activation='relu'),
                keras.layers.Dense(128, activation='relu'),
                keras.layers.Dense(8)
            ]
        )
        
        opt = keras.optimizers.Adam(learning_rate=0.001)
        
        self.model.compile(optimizer=opt, 
            loss=['mse'], 
            metrics=['accuracy', 'mse', 'binary_crossentropy'])
        self.model.summary()

    def predict(self, input: np.ndarray):
        return self.model.predict(expand_dims(input, axis=0))

    # Trains the model using supervised learning
    def train(self, inputs: np.ndarray, outputs: np.ndarray):
        self.model.fit(inputs, outputs, epochs=100, batch_size=32)
    
    def save_model(self, name=datetime.now().strftime('%Y-%m-%d_%H-%M')):
        full_path = '{}/../saved_models/{}'.format(os.path.dirname(os.path.realpath(__file__)), name)
        self.model.save(full_path)

    def load_model(self, name):
        full_path = '{}/../saved_models/{}'.format(os.path.dirname(os.path.realpath(__file__)), name)
        self.model = keras.models.load_model(full_path)
