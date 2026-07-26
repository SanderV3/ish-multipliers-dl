"""Template for a custom ViT evaluated through TransAxx — copy to <name>.py.

There is NO LOCAL_MODELS hook for the ViT sweep. The local-model hook is wired
into the AdaPT sweep only (see 0_configure_sweeps' "Extending the pipeline" cell
for why). So a custom ViT does not get registered in 0_configure_sweeps — it gets
its own notebook, copied from 1_run_sweep_transaxx_vit, with the three
model-specific pieces below pulled out into this file so the copied notebook
stays close to the original.

You do NOT need this for a stock timm model: a timm ImageNet-1k name just goes in
TRANSAXX_VIT.models and the sweep resolves everything from timm. This file is for
a ViT that timm cannot build (your own architecture/weights), or one that needs a
non-timm eval transform.

The contract is wider than the AdaPT _template.py, because the timm path in the
ViT sweep derives two things from the model object that a custom model has to
supply itself:

    build_model()      -> eval-ready nn.Module, weights loaded, on CPU
                          (the notebook moves it to the GPU)
    make_transform()   -> the eval-time torchvision transform with the right
                          mean/std/crop/interpolation FOR THIS model
    approx_layers(m)   -> the Linear layers to approximate; the default mirrors
                          1_run_sweep_transaxx_vit's linear_layers_of (every
                          Linear except the classifier head / patch reduction)

In the copied notebook, call these in place of its build_model / make_transform /
linear_layers_of cells.
"""
from pathlib import Path

import torch
import torch.nn as nn

_HERE = Path(__file__).parent   # weights live next to this file


def build_model():
    """Return the model with weights loaded, in eval mode, on CPU.

    Paste or import the real architecture so it matches the .pth state_dict
    exactly. Leave it on CPU — the sweep calls .to(device) itself.
    """
    raise NotImplementedError("build and load the real ViT here")
    # model = MyViT(...)
    # model.load_state_dict(torch.load(_HERE / "my_vit.pth", map_location="cpu"))
    # return model.eval()


def make_transform():
    """Return the eval-time transform for this model (not the training one).

    A stock timm model lets timm resolve this; a custom model has to state it.
    Match the mean/std/crop/interpolation the weights were trained with, or the
    accuracy reading is meaningless. The values below are the ImageNet defaults
    at 224 — change them if your model differs.
    """
    import torchvision.transforms as T
    size, crop = 224, 0.9
    return T.Compose([
        T.Resize(int(round(size / crop)), interpolation=T.InterpolationMode.BICUBIC),
        T.CenterCrop(size),
        T.ToTensor(),
        T.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
    ])


def approx_layers(model):
    """Return the (name, module) Linear layers to approximate.

    The sweep counts these to size the per-layer axx_list and excludes the same
    names in replace_linear_layers, so this must agree with what the sweep
    replaces. Default = every nn.Linear except the classifier head and any patch
    'reduction', matching 1_run_sweep_transaxx_vit. Override if your model names
    its head differently, or to approximate a different subset.
    """
    return [(n, m) for n, m in model.named_modules()
            if isinstance(m, nn.Linear) and "head" not in n and "reduction" not in n]
