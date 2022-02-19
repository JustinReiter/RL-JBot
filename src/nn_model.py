import numpy as np
import os
from datetime import datetime

import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2' 

from tensorflow import keras, expand_dims

class NN_Model:

    def __init__(self):
        self.inputs = keras.Input(shape=(67,))
        self.intermediate_layers = [keras.layers.Dense(256, activation='tanh')(self.inputs)]
        for i in range(3):
            self.intermediate_layers.append(
                keras.layers.Dense(256, activation='tanh')(self.intermediate_layers[i])
            )

        self.main_output_layer = keras.layers.Dense(2, activation='tanh')(self.intermediate_layers[-1])
        self.boost_jump_output_layer = keras.layers.Dense(2, activation='sigmoid')(self.intermediate_layers[-1])
        self.controls_output_layer = keras.layers.Dense(3, activation='tanh')(self.intermediate_layers[-1])
        self.handbrake_output_layer = keras.layers.Dense(1, activation='sigmoid')(self.intermediate_layers[-1])

        self.combined_output = keras.layers.Concatenate()([
                self.main_output_layer,
                self.boost_jump_output_layer,
                self.controls_output_layer,
                self.handbrake_output_layer
            ]
        )
        
        self.model = keras.Model(self.inputs, outputs=self.combined_output)
        opt = keras.optimizers.Adam(learning_rate=0.00001)
        self.model.compile(optimizer=opt, 
            loss=['mse', 'binary_crossentropy', 'mse', 'binary_crossentropy'],
            metrics=['accuracy', 'mse', 'binary_crossentropy'])
        self.model.summary()

    def predict(self, input: np.ndarray):
        return self.model.predict(expand_dims(input, axis=0))

    # Trains the model using supervised learning
    def train(self, inputs: np.ndarray, outputs: np.ndarray):
        self.model.fit(inputs, outputs, epochs=100, batch_size=60)
    
    def save_model(self, name=datetime.now().strftime('%Y-%m-%d_%H-%M')):
        full_path = '{}/../saved_models/{}'.format(os.path.dirname(os.path.realpath(__file__)), name)
        self.model.save(full_path)

    def load_model(self, name):
        full_path = '{}/../saved_models/{}'.format(os.path.dirname(os.path.realpath(__file__)), name)
        self.model = keras.models.load_model(full_path)
