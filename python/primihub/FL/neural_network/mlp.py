from torch import nn


# Multi_layer percepton demo
class NeuralNetwork(nn.Module):

    def __init__(self, output_dim):
        super().__init__()
        self.layers = nn.Sequential(
            nn.LazyLinear(32),
            nn.ReLU(),
            nn.LazyLinear(16),
            nn.ReLU(),
            nn.LazyLinear(output_dim)
        )

    def forward(self, x):
        logits = self.layers(x)
        return logits
