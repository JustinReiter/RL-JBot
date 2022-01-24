import numpy as np


class NN_Model:

    LEARNING_RATE = 0.1
    LAYERS = 2

    @staticmethod
    def cost_mse(prediction: float):
        return (prediction - NN_Model.TARGET) ** 2
    
    @staticmethod
    def sigmoid(x: float):
        return 1 / (1 + np.exp(-x))

    @staticmethod
    def sigmoid_deriv(x: float):
        return 

    def __init__(self):
        self.weight = np.array([])
        self.bias = 0
        self.target = 0

    # squared error used --> d/dx (x^2) = 2 * x
    def perform_gradient_descent(self, layer_results, input):
        derror = 2 * NN_Model.cost_mse(layer_results[-1] - self.target)
        dlayer1 = NN_Model.sigmoid_deriv(layer_results[-2])
        dbias = 1
        dweights = input

        return [dbias, dweights]

    def adjust_parameters(self, derror):
        self.bias -= - derror[0]
        self.weights -= derror[1]


    def predict(self, input: np.ndarray):
        # Perform calculations for layers

        layer_results = [0] * NN_Model.LAYERS

        # First layer
        layer_results[0] = self.weights * input + self.bias

        # Last layer -- sigmoid
        layer_results[1] = NN_Model.sigmoid(layer_results[0])

        return layer_results[1]
    
    def train(self, input: np.ndarray):
        # Perform calculations for layers

        layer_results = [0] * NN_Model.LAYERS

        # First layer
        layer_results[0] = self.weights * input + self.bias

        # Last layer -- sigmoid
        layer_results[-1] = NN_Model.sigmoid(layer_results[-2])

        # Perform gradient descent
        error = self.perform_gradient_descent(layer_results)
        self.adjust_parameters(error)
