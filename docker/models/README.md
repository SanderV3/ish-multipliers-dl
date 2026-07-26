# Local models

Drop a model you have on disk (e.g. an externally provided `.pth`) here and the
AdaPT sweep can run it through the same multiplier matrix as the built-in models.

A `.pth` file alone is **not** enough: it only holds the weights (a
`state_dict`), not the architecture. You need two files per model:

```
docker/models/
├── <name>.py     <- defines build_model() -> eval-ready nn.Module (see _template.py)
└── <name>.pth    <- the weights, loaded by <name>.py itself
```

Then register it in `0_configure_sweeps.ipynb`:

```python
LOCAL_MODELS = {
    "<name>": {"framework": "adapt", "dataset": "cifar10"},
}
```

Run All in `0_configure_sweeps`, then re-run `1_run_sweep_adapt`. The model is
swept on its declared dataset only (a local model is trained for one dataset;
running it on another would be meaningless).

The contract for `<name>.py` (full example in `_template.py`):

- `build_model()` returns an `nn.Module` with weights loaded, in eval mode.
- `forward` returns **one** logits tensor of shape `[batch, n_classes]` (the
  generic sweep's `evaluate()` assumes a single output).
- Loadable on **CPU PyTorch 1.13** (AdaPT's environment).

If a model genuinely has multiple outputs (an early-exit net, say) or a
non-standard input pipeline, it doesn't go through
`LOCAL_MODELS`/`1_run_sweep_adapt`. It gets its own sweep notebook, copied from
`../notebooks/_template_sweep.ipynb` (three `TODO` blocks: the model, the
loaders, the scoring). Its files still live in this directory — the `.py`, any
helper modules it imports, the weights — but the scoring that the generic
`evaluate()` can't express moves into that notebook. Write the same `runs.csv`
columns the template already writes and the analysis notebooks pick the model up
with no changes.

A custom **ViT** skips this hook too, but for a different reason: the local-model
hook is AdaPT-only, so the ViT sweep has nothing to register it with. A stock timm
model just goes in `TRANSAXX_VIT.models` in `0_configure_sweeps`. A ViT that timm
cannot build gets its own notebook copied from `1_run_sweep_transaxx_vit`.
`_template_vit.py` is the contract stub for that case: `build_model()` plus the
eval transform and the layer list the timm path would otherwise have supplied.

This directory is bind-mounted into the containers at `/work/models`
(see `docker-compose.yml`). The `.py` files are committed. Weights (`*.pth`,
`*.pt`) are gitignored, so copy them onto each machine that runs the sweep.
