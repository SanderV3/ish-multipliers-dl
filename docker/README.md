# Docker pipeline: approximate-multiplier DNN evaluation

A reproducible, extensible container setup for evaluating 8-bit approximate
multipliers (`FPGA_ISH_1..5`) on CNN and ViT inference. Three services:

| Service    | Image         | Hardware | Port | What it does |
|------------|---------------|----------|------|--------------|
| `adapt`    | `rp-adapt`    | CPU      | 8888 | AdaPT sweeps (CIFAR CNNs) |
| `transaxx` | `rp-transaxx` | **GPU**  | 8889 | TransAxx sweeps (CIFAR CNNs, ImageNet ViTs) |
| `analysis` | `rp-analysis` | CPU      | 8890 | Figures, tables, and the NAME (paper Table 6) reproduction |

## Design: environment in the image, code + data mounted

The two framework images are heavy (CPU PyTorch; CUDA + GPU PyTorch). The
analysis image is slim (no framework), so you don't boot a 16 GB CUDA image just
to draw a chart or recompute a metric, and the analysis container can see both
frameworks' results at once for the AdaPT-vs-TransAxx comparison.

Notebooks and results are bind-mounted, not baked into the images. Only the
pinned environment lives in the image. The notebooks (your code) and the
results/datasets (your data) are mounted at run time. So you can edit a parameter
in JupyterLab and re-run with no rebuild, and there's one copy of each notebook
(`docker/notebooks/`) instead of a copy baked into every image. This follows the
approach in Nüst et al. (2020), *Ten Simple Rules for Writing Dockerfiles for
Reproducible Data Science* (PLOS Comput Biol 16(11):e1008316): rule 7 (mount
code/data at run time), rule 2 (build on existing images), rule 5 (pin versions),
rule 8 (one-click runnable).

In-container layout (identical across services):

```
/work/                      <- Jupyter root (WORKDIR)
  notebooks/   <- bind-mount  docker/notebooks/   (live-editable; holds sweep_config.json)
  results/     <- bind-mount  docker/results/     (adapt/ + transaxx/)
  luts/        <- bind-mount  docker/luts/        (multiplier headers + manifest)
  models/      <- bind-mount  docker/models/      (your own model files, optional)
/workspace/multipliers/     <- baked in (C sources + canonical reference LUTs)
/workspace/datasets/imagenet_data/  <- bind-mount (transaxx only; ViT sweep)
```

Each notebook's kernel runs with cwd `/work/notebooks`, so it writes/reads
results via `../results/<framework>` and multiplier headers via `../luts/`.

### Multipliers are a bind-mount too (single source of truth)

`docker/luts/<framework>/` is mounted **over** each framework's JIT kernel
directory (AdaPT's `cpu-kernels/axx_mults/`, TransAxx's
`ext_modules/include/nn/cuda/axx_mults/`). So the headers written by
`0_build_multipliers.ipynb` *are* what the sweeps compile against. Define or
change a multiplier in that notebook, and it's usable on the next sweep with no
image rebuild. The C pipeline under `/workspace/multipliers/` stays baked as the
canonical reference that the notebook's self-check validates against (and that
notebook 4 reads for LUT metrics).

### One place to choose models, datasets and parameters

`0_configure_sweeps.ipynb` is the single editable surface for *what* gets
evaluated. Edit the dicts (`ADAPT`, `TRANSAXX`, `TRANSAXX_VIT`, `LOCAL_MODELS`)
and Run All. It validates your choices and writes `sweep_config.json` next to the
notebooks, then you run a sweep notebook, which reads that file. A typo (an
unavailable CIFAR model, a wrong dataset, a local model with no `.py`) fails in
seconds in the config notebook instead of an hour into a run. The destructive
`FRESH` toggle deliberately stays inside each sweep notebook, so a shared config
file can never wipe results as a side effect. The CIFAR sweeps are list-checked
against `chenyaofo/pytorch-cifar-models`. The ViT sweep takes any
ImageNet-pretrained timm name (its preprocessing is resolved automatically).

### Bringing your own model (`docker/models/`)

To sweep a model you have on disk (e.g. an externally provided `.pth`), drop two
files in `docker/models/`: `<name>.py` (defining `build_model()`, copy
`_template.py`) and the weights it loads. Then register it in
`0_configure_sweeps`'s `LOCAL_MODELS`. A bare `.pth` is only weights, not the
architecture, which is why the `.py` is required. `build_model()` must return an
eval-ready model whose `forward` yields one logits tensor. Local models currently
run through the **AdaPT** sweep on their declared dataset only. The `.py` files are
committed. The weight blobs (`*.pth`, `*.pt`) are gitignored, so copy them onto each
machine. Full contract in `docker/models/README.md`.

A model that doesn't fit the one-logits-tensor contract, because it has multiple
outputs (an early-exit net, say) or a non-standard input pipeline, doesn't go
through `LOCAL_MODELS` at all. It gets its own sweep notebook instead, copied
from `notebooks/_template_sweep.ipynb`: its `.py` and weights still live in
`docker/models/`, but the model-specific scoring lives in that notebook. The
template is the standard AdaPT sweep (same wrap, same calibration, same
`runs.csv` schema) with the model, the loaders and the scoring left as three
marked `TODO`s.

## Run it

```bash
# from the repo root
docker compose -f docker/docker-compose.yml up adapt        # -> http://localhost:8888
docker compose -f docker/docker-compose.yml up transaxx     # -> http://localhost:8889
docker compose -f docker/docker-compose.yml up analysis     # -> http://localhost:8890
```

Open the URL (no token/password, local use only). To drive these from another
machine see [Running on a remote GPU host](#running-on-a-remote-gpu-host).
Notebook order:

0. `0_build_multipliers`: **define/add/change the multipliers** (edit three
   tables, Run All). Writes the LUT headers + `multipliers.json` manifest and
   self-checks against the C ground truth. Run in `analysis` (pure Python, no GPU).
0. `0_configure_sweeps`: **choose models / datasets / parameters** (and register
   your own models). Validates and writes `sweep_config.json`. Run in `analysis`.
1. `1_run_sweep_adapt` / `1_run_sweep_transaxx` / `1_run_sweep_transaxx_vit`:
   run a sweep. Reads `sweep_config.json` + the multiplier manifest, writes
   `results/<framework>/runs.csv` + confusion matrices.
2. `2_accuracy_comparison`: tables + charts across every multiplier (set
   `FRAMEWORK` / `DATASET`).
3. `3_multiplier_deepdive`: one multiplier in depth.
4. `4_multiplier_metrics`: NAME (paper Table 6) + LUT error metrics. No sweep
   needed, no GPU.
5. `5_arch_comparison`: CNN-vs-ViT, the approximate-multiplier accuracy drop
   (vs exact int8, eval-only) for CNNs (AdaPT) vs ViTs (TransAxx), plus the
   ViT-only fp32→int8 PTQ drop. Reads both `runs.csv`, writes `results/comparison/`.
   Run in `analysis`.

Run notebook 0 + 2–5 in `analysis` (lighter, sees both frameworks' results). Run
the sweeps in the `adapt`/`transaxx` services. To add a multiplier mid-session:
edit `0_build_multipliers` in `analysis`, then re-run a sweep. The new header is
already mounted into the framework container.

## Windows / Docker Desktop

The pipeline is developed on Linux but builds and runs on Windows through Docker
Desktop (WSL2 backend). Three things to know:

- **Clone inside WSL2, not on `C:\`.** Put the repo under the Linux filesystem
  (a `/home/<you>/...` path from a WSL shell) and run `docker compose` from
  there, **from the repo root**. Windows-filesystem bind mounts are slow and map
  permissions lossily.
- **Line endings are handled for you.** `.gitattributes` forces LF on checkout,
  so the in-image `build_all.sh` runs even though Git for Windows would otherwise
  rewrite it to CRLF (which breaks `bash` and fails the build). This only applies
  to clones made *after* `.gitattributes` was committed, so an older checkout needs
  a fresh clone or `git add --renormalize .`.
- **GPU sweeps (`transaxx`), two routes.**
  - *Local NVIDIA GPU:* Docker Desktop exposes it through WSL2 (install the
    Windows NVIDIA driver, no extra container toolkit needed). The base
    `deploy.resources` block is correct, so do **not** add `docker-compose.cdi.yml`
    on Windows. That override is Linux/CDI only.
  - *Remote university GPU:* don't run it on the laptop at all, see [Running on
    a remote GPU host](#running-on-a-remote-gpu-host). Your Windows machine is
    only the SSH client. The build and GPU run happen on the Linux host.

  With no GPU at all, skip `transaxx` (it errors `could not select device driver
  ... [[gpu]]`): the ViT results are already committed, so `analysis` shows
  them with no re-run.

## GPU via CDI (Container Device Interface)

Some hosts hand out the GPU over the **Container Device Interface** instead of
the legacy nvidia-container-runtime. This is the default on NixOS, on Podman-based
setups, and on recent Docker Engine / Fedora-style hosts that are configured for
CDI. The tell is that the base `transaxx` service can't see the GPU even though
`nvidia-ctk cdi list` shows a `nvidia.com/gpu` device. On such a host, chain the
override onto `transaxx`:

```bash
docker compose -f docker/docker-compose.yml -f docker/docker-compose.cdi.yml up transaxx
```

The override (`docker-compose.cdi.yml`) resets the base `deploy` block and uses a
`nvidia.com/gpu=all` CDI device instead. Windows / Docker Desktop and standard
Linux on the nvidia runtime use the base file as-is (no override). CDI is a
*runtime* concern only, it never affects `docker build`.

## Running on a remote GPU host

The GPU sweep (`transaxx`) is the heavy one, so you'll often run it on a remote
machine, like a lab desktop or a cloud GPU box, while driving JupyterLab from your
laptop.

> **The remote host must let you run Docker.** A personal lab desktop or a cloud
> GPU VM is fine. Shared university **HPC / SLURM clusters usually disable Docker**
> for security and provide Apptainer/Singularity instead, which *can* run this
> image (converted to a `.sif`, launched with `apptainer exec --nv`) but **not**
> via `docker compose`. Confirm which kind of host you have before relying on the
> commands below. The Apptainer path needs a different launch recipe (not yet
> scripted here). Because the environment lives in the image and only the notebooks/results
are bind-mounted, this is the *same* setup, not a different one: run the same
compose on the remote host, then forward the port over SSH.

```bash
# on the remote GPU host (clone the repo there first)
docker compose -f docker/docker-compose.yml -f docker/docker-compose.cdi.yml up transaxx
# on your laptop
ssh -L 8889:localhost:8889 user@remote-host
# now open http://localhost:8889 locally, served by the remote GPU
```

(Drop the `docker-compose.cdi.yml` override if the remote host uses the standard
nvidia runtime rather than CDI, see [GPU via CDI](#gpu-via-cdi-container-device-interface). The same
tunnel pattern drives `adapt`/`analysis` on their ports, `8888`/`8890`.)

The published ports are bound to `127.0.0.1` (e.g. `127.0.0.1:8889:8889` in
`docker-compose.yml`) **on purpose**: JupyterLab runs with no token and no
password, which is fine on loopback but would be unauthenticated remote code
execution if exposed to a network. Loopback-binding forces remote access through
the SSH tunnel above and never puts the tokenless server on a reachable
interface. Local use (`http://localhost:8889` on the host itself) is unaffected.

Stage whatever the sweep reads on the remote host. For the ViT sweep that's
ImageNet under `docker/datasets/` (gitignored; see below). Results land in the
bind-mounted `docker/results/`, so commit + pull (or `rsync`) them back to render
the tables/figures on a GPU-less machine.

## Staging ImageNet (ViT sweep)

ImageNet is **not** shipped with this repo and can't be: its terms of access
forbid redistribution, and ImageNet doesn't own the underlying image copyrights,
so anyone re-running the ViT sweep downloads it themselves under their own
agreement to those terms. (CIFAR-10/100, which *are* freely redistributable,
auto-download into `docker/datasets/` instead.)

The ViT sweep reads `docker/datasets/imagenet_data/imagenet/{val,train_tiny}/`
(mounted read-write into `transaxx`). Stage the real **ILSVRC2012** validation set
once: download `ILSVRC2012_img_val.tar` (6.3 GB) + `ILSVRC2012_devkit_t12.tar.gz`
(2.5 MB) from image-net.org into `docker/datasets/imagenet_data/`, then set
`PREP_IMAGENET_REAL = True` in `1_run_sweep_transaxx_vit.ipynb` and run it once. It
uses `torchvision.datasets.ImageNet` to parse the devkit and lay the images out in
`imagenet/val/<WNID>/` in timm's class order, then carves a small `train_tiny/`
calibration slice. `docker/datasets/` is gitignored.

`shm_size: 8gb` on `transaxx` is required, because the ImageNet DataLoader blows
past Docker's default 64 MB `/dev/shm`.

## Notes

- **Notebook outputs are committed.** Notebooks are committed with their cell
  outputs, so executed results travel across machines (run the GPU sweeps on a
  desktop, then view the rendered tables/charts on a GPU-less laptop) and render
  inline on GitHub. The trade-off is noisier diffs. If you later want clean diffs
  instead, install [`nbstripout`](https://github.com/kynan/nbstripout)
  (`pip install nbstripout`) and run `nbstripout --install --attributes .gitattributes`
  in the repo to strip outputs on commit.
- **Frozen artifact.** For a snapshot you can rebuild byte-for-byte, `docker build`
  and tag a release image at a commit (notebooks baked at that point). You keep
  both: the mounted tool for day-to-day, and a frozen tag for the record.
- **Pinning.** All three environments are version-pinned for the frozen artifact
  (rule 5): `requirements-{adapt,transaxx}.txt` for the frameworks, and
  `requirements-analysis.txt` now carries the full `pip freeze` of `rp-analysis`
  (direct deps first, transitive closure below). To bump, edit a direct dep,
  rebuild, and re-freeze.
