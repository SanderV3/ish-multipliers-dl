# Approximate-multiplier DNN evaluation (AdaPT + TransAxx)

This repo evaluates five 8-bit approximate multipliers, ISH-1..ISH-5 (code name
`FPGA_ISH_1`..`FPGA_ISH_5`), on CNN and ViT inference. It is the evaluation
pipeline behind the paper *Evaluating FPGA-Optimized Power-Efficient Internal
Self-Healing-Based Approximate Multipliers in Larger Deep Learning Workloads*
(Marki-Pleym, Rasheed, and Gillani, University of Twente, submitted to FIT 2026). A small C pipeline turns each multiplier design into a
256×256 lookup-table (LUT) header, and two emulation frameworks then run DL
inference that multiplies through those LUTs instead of an exact multiplier.

The first one is **AdaPT**, the CPU emulator, which runs the CIFAR CNNs. The
second one is **TransAxx**, the GPU emulator, which runs the ImageNet ViTs.

Everything runs inside Docker. Nothing has to be installed on the host beyond
Docker itself, and for the GPU image an NVIDIA driver (see [GPU prerequisites](#gpu-prerequisites)).
If you just want the "what is this and why" story rather than the run
instructions, read [`OVERVIEW.md`](OVERVIEW.md) first.

## Quick start

```bash
git clone <this-repo>
cd <this-repo>

# from the repo root
docker compose -f docker/docker-compose.yml up adapt      # CPU  -> http://localhost:8888
docker compose -f docker/docker-compose.yml up transaxx   # GPU  -> http://localhost:8889
docker compose -f docker/docker-compose.yml up analysis   # CPU  -> http://localhost:8890
```

Pick the service you need. You don't run all three at once. Each command builds
the image the first time, then starts a JupyterLab server and stays in the
foreground (it holds that terminal), so give each one its own terminal, or add
`-d` to run it in the background. Stop it with Ctrl-C. Open the URL it prints (no
token, local use only). The notebooks are under `notebooks/`. The three services
are:

| Service    | Hardware | Port | What it does |
|------------|----------|------|--------------|
| `adapt`    | CPU      | 8888 | AdaPT sweeps of the CIFAR CNNs |
| `transaxx` | **GPU**  | 8889 | TransAxx sweeps of CIFAR CNNs and ImageNet ViTs |
| `analysis` | CPU      | 8890 | Tables, figures, and the multiplier-error metrics |

The first build is the slow one (roughly 5–20 minutes): it installs the
toolchain, pulls the PyTorch wheels, clones the upstream framework, and builds
the LUTs. After that the image is cached and starts fast.

The notebooks are numbered in run order. The short version: `0_*` define the
multipliers and choose what gets swept, `1_*` run a sweep, `2_`–`5_` turn the
sweep output into the tables and figures. The full walkthrough is in
[`docker/README.md`](docker/README.md): every notebook, the bind-mount design,
and how to add your own multiplier, model, or dataset. That file is the canonical
reference for the pipeline. This one is just the front door.

## GPU prerequisites

The `transaxx` image is the only one that needs a GPU. It wants:

- An NVIDIA driver new enough for CUDA 11.7 (≥ 515.x).
- [`nvidia-container-toolkit`](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html)
  on the host, so Docker can hand the GPU to the container. On Windows this ships
  with a recent Docker Desktop on the WSL2 backend.
- A GPU from the Turing, Ampere, or Ada generations (RTX 20/30/40 series and the
  matching Quadro / Tesla / A100 cards). TransAxx is pinned to PyTorch 1.13 +
  CUDA 11.7, and those prebuilt wheels only cover compute capabilities sm_37
  through sm_86. Blackwell (RTX 50, sm_120) and newer would need the framework
  ported to PyTorch 2.x + CUDA 12.x first.

`adapt` and `analysis` are CPU-only, so you can do everything except the ViT
sweeps with no GPU at all. The committed notebook outputs already carry the ViT
results, so `analysis` renders them without a re-run.

Some hosts hand out the GPU over **CDI** (Container Device Interface) instead of
the legacy nvidia runtime. NixOS and Podman-based setups are the common ones. On
those, chain the CDI override onto `transaxx`:

```bash
docker compose -f docker/docker-compose.yml -f docker/docker-compose.cdi.yml up transaxx
```

Windows / Docker Desktop and the standard Linux nvidia runtime use the base
compose file as-is, no override. CDI is a *runtime* concern, it never changes the
build. There is more on the GPU paths (Windows, remote hosts, CDI) in
[`docker/README.md`](docker/README.md).

## Staging ImageNet (for the ViT sweep)

The ViT sweep (`transaxx`) runs on ImageNet, and ImageNet can't ship inside
the repo: its terms forbid redistribution, so whoever runs that sweep downloads it
themselves under their own agreement to those terms. CIFAR-10/100 are fine to
redistribute and auto-download into `docker/datasets/` on first run, so the CNN
sweeps need none of this. This section is only for the ViTs.

**Which.** The ILSVRC2012 validation set. Make a free account on
[image-net.org](https://image-net.org/), accept the terms, and grab two files:

- `ILSVRC2012_img_val.tar`, the 50k validation images (~6.3 GB).
- `ILSVRC2012_devkit_t12.tar.gz`, the devkit that holds the labels (~2.5 MB).

**Where.** Drop both into `docker/datasets/imagenet_data/` on the machine that
runs `transaxx`. That folder is gitignored, so the data never lands in the repo.

**How.** Set `PREP_IMAGENET_REAL = True` in
`docker/notebooks/1_run_sweep_transaxx_vit.ipynb` and run it once. That cell uses
`torchvision.datasets.ImageNet` to read the devkit and lay the images out as
`imagenet/val/<WNID>/` in the class order timm expects, then carves off a small
`train_tiny/` slice for the int8 calibration. After that the sweep finds the data
and runs. You only do this once per machine.

You only need ImageNet for the ViT numbers. With no GPU or no ImageNet, skip
`transaxx`. The committed notebook outputs already carry the ViT results, so
`analysis` shows them with no re-run. The full detail (including the `shm_size`
requirement on the GPU service) is in [`docker/README.md`](docker/README.md).

## Repo layout

- `multipliers/` holds the C pipeline that composes the multipliers (2-bit
  primitives → 4-bit blocks → 8-bit multipliers → LUT header) plus its tests.
  This is the original work. The two frameworks are third-party.
- `docker/` holds the Dockerfiles, the compose file, the notebooks, and the
  bind-mount targets (`luts/`, `results/`, `models/`, `datasets/`). Read
  `docker/README.md` for the pipeline guide.
- `requirements-adapt.txt` / `requirements-transaxx.txt` / `requirements-analysis.txt`
  are the pinned pip deps that go into each image.

The early-exit (BranchyAlexNet) model and checkpoint behind the paper's
early-exit results are not distributed here: that network is not yet
publishable. `docker/notebooks/_template_sweep.ipynb` shows how such an
off-contract model is swept through the same pipeline.

AdaPT and TransAxx are not vendored here. The Dockerfiles clone them from
upstream at build time, so the heavy framework code stays out of this repo and
you always know exactly which upstream commit an image was built from (pin it
with `--build-arg ADAPT_REF=<sha>` / `TRANSAXX_REF=<sha>` for a reproducible
image).

## Use of AI tools

This project used an AI coding assistant (a large language model) as a tool on
the development side: setting up and debugging the C LUT pipeline, the Docker
setup, and the analysis notebooks, and wiring the AdaPT and TransAxx pieces
together. The experiment design, the analysis, and the conclusions are the author's own,
and every reported number was checked against the raw run data rather than
taken on trust. This note keeps that disclosure with the code.
