"""Template for a local (user-provided) model — copy to <name>.py.

The sweep imports this file and calls build_model(); everything
architecture-specific (the class, the weight loading, any output selection)
stays in here, so the sweep notebooks never need editing for a new model.

Register the model in 0_configure_sweeps.ipynb:
    LOCAL_MODELS = {"<name>": {"framework": "adapt", "dataset": "cifar10"}}
"""
from pathlib import Path

import torch
import torch.nn as nn

_HERE = Path(__file__).parent   # weights live next to this file


class MyNet(nn.Module):
    """Paste the real architecture here (or import it from another file
    in this directory). It must match the state_dict in the .pth exactly."""

    def __init__(self, n_classes=10):
        super().__init__()
        raise NotImplementedError("replace MyNet with the actual model class")

    def forward(self, x):
        raise NotImplementedError


def build_model():
    model = MyNet()
    state = torch.load(_HERE / "my_net.pth", map_location="cpu")
    model.load_state_dict(state)
    # Early-exit / multi-output models: forward must return ONE logits tensor
    # of shape [batch, n_classes]. If yours returns several exits, wrap it:
    #
    #   class FinalExitOnly(nn.Module):
    #       def __init__(self, inner): super().__init__(); self.inner = inner
    #       def forward(self, x): return self.inner(x)[-1]
    #
    #   model = FinalExitOnly(model)
    model.eval()
    return model
