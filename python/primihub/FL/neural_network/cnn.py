from torch import nn


# Convolutional neural network demo
class NeuralNetwork(nn.Module):

    def __init__(self, output_dim):
        super().__init__()
        self.layers = nn.Sequential(
           nn.LazyConv2d(16, 8, 2, 2),
           nn.ReLU(),
           nn.MaxPool2d(2, 1),
           nn.LazyConv2d(32, 4, 2, 0),
           nn.ReLU(),
           nn.MaxPool2d(2, 1),
           nn.Flatten(),
           nn.LazyLinear(32),
           nn.ReLU(),
           nn.LazyLinear(output_dim) 
        )

    def forward(self, x):
        logits = self.layers(x)
        return logits
