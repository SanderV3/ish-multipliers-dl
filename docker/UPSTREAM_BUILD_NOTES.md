# Building the upstream frameworks directly (AdaPT & TransAxx)

**TL;DR — you don't need to.** This repo ships working, modern Docker images for
both frameworks. One command each:

```bash
docker compose -f docker/docker-compose.yml up adapt      # CPU  -> http://localhost:8888 (Jupyter Lab, no token)
docker compose -f docker/docker-compose.yml up transaxx   # GPU  -> http://localhost:8889
# GPU on a CDI host (NixOS/Podman/recent Docker): add the override
docker compose -f docker/docker-compose.yml -f docker/docker-compose.cdi.yml up transaxx
```

These notes are only for the case where you want to build the *original* upstream
repos yourself. Read the summary first — the short version is that AdaPT's
upstream build is broken today and TransAxx's targets a different PyTorch than
the paper, and this repo's images fix/avoid both.

---

## What this repo changed vs upstream, and why (the short version)

| | **AdaPT** | **TransAxx** |
|---|---|---|
| Upstream base | `ubuntu:20.04`, Python 3.8, torch 1.8.1 | `pytorch/pytorch:2.0.1-cuda11.7`, torch **2.0.1** |
| This repo's base | `ubuntu:22.04`, Python 3.10, torch 1.13.1 | `nvidia/cuda:11.7.1`, torch **1.13.1+cu117** |
| Main reason | upstream **no longer builds** (see below) | **match the paper** — TransAxx was evaluated on torch 1.13 / CUDA 11.7, not 2.0.1 |
| Build-tool fixes | pin `notebook<7`; `cd` before `pytorch-quantization` install | pin `setuptools<70` and `pip<24` (torch 1.13's JIT needs them); install the vendored `pytorch-quantization` editable, `--no-build-isolation` |
| pytorch-quantization | PyPI wheel `2.1.3` (clean) | **vendored fork**, editable — TransAxx's CUDA layers need that exact fork, not PyPI |
| Same wrapping (both) | Jupyter **Lab** auto-starts (token-less); notebooks/results/LUTs **bind-mounted** (edit + re-run, no rebuild); the LUT dir is mounted **over** the framework's JIT-kernel path so a new multiplier needs no rebuild; one `compose` command | (same) |

**In one sentence:** for AdaPT we had to *repair a broken build*; for TransAxx we
had to *pin it back to the versions the paper used* (and deal with the build-tool
fallout of using torch 1.13). On top of both, we added the same reproducibility
wrapping (one-command Lab, bind mounts, LUT-as-single-source-of-truth).

### Why those specific pins exist

- **`notebook<7` (AdaPT):** unpinned `notebook` now pulls Notebook 7 → needs
  JupyterLab 4.4.x → which dropped Python 3.8, but the base is Python 3.8. So the
  build dies. Pinning keeps classic Notebook 6.x, which runs on 3.8.
- **`setuptools<70` (TransAxx):** torch 1.13's `cpp_extension.py` does
  `from pkg_resources import packaging`, which setuptools 70+ removed. Without the
  pin, the CUDA-kernel JIT builds (the whole point of TransAxx) fail.
- **`--no-build-isolation` (TransAxx):** the vendored `pytorch-quantization`'s
  `setup.py` imports torch at build time, which fails inside pip's isolated build
  env, so we install in the existing env.
- **`cd` before `setup.py install` (AdaPT):** upstream runs it from `/`, so
  `find_packages()` finds nothing and only the C extension installs — the Python
  package is silently skipped and `from pytorch_quantization import nn` fails.

---

## AdaPT — building the upstream repo directly

Tested on x86_64 Linux, Docker 29.x. A fresh `git clone
https://github.com/dimdano/adapt.git` **does not build** without these two edits
to `adapt/docker/AdaPT.Dockerfile`:

```dockerfile
# (1) ~line 18 — pin Jupyter off the Python-3.8-incompatible JupyterLab 4
- RUN pip3 install notebook absl-py>=0.7.0 scipy sphinx_glpi_theme prettytable pyyaml tqdm ninja cython ipywidgets
+ RUN pip3 install "notebook<7" "absl-py>=0.7.0" scipy sphinx_glpi_theme prettytable pyyaml tqdm ninja cython ipywidgets

# (2) ~line 22 — cd in first, or only the C extension installs
- RUN python3 /etc/pytorch-quantization/setup.py install
+ RUN cd /etc/pytorch-quantization && python3 setup.py install
```

### Run it (portable commands; the upstream `*.sh` scripts are bash-only)

```bash
# 1. build (context is the docker/ folder)
docker build -f adapt/docker/AdaPT.Dockerfile -t dimdano/adapt-cpu:0.1 adapt/docker

# 2. start the container from the repo root (adapt/), mapping the port
#    PowerShell:  -v ${PWD}:/workspace/adapt
docker run --rm -it -p 8888:8888 -v "$(pwd)":/workspace/adapt -w /workspace/adapt dimdano/adapt-cpu:0.1 bash

# 3. inside that shell, start Jupyter (classic Notebook 6.x, not Lab)
jupyter notebook --no-browser --allow-root --ip=0.0.0.0 --port=8888
```

Open the printed URL, replacing the container hostname with `localhost`
(`http://localhost:8888/?token=...`). The example notebooks are under
`examples/`; they need pretrained weights from the Google Drive link in the
upstream README, placed in `examples/models/state_dicts/`.

Unlike this repo's compose, building/running starts **nothing** — you get a bash
shell and launch Jupyter (classic Notebook, token-protected) by hand.

### Windows gotchas

1. **`.sh` scripts don't run in PowerShell/CMD** — use the `docker` commands
   above, or run the scripts from Git Bash / WSL2.
2. **`--network=host` (used by upstream's `run_docker.sh`) does not publish ports
   on Docker Desktop** → `localhost:8888` would never connect. Use `-p 8888:8888`.
3. **`--ip=0.0.0.0` is required with `-p`** — upstream's `run_jupyter.sh` omits
   it, so Jupyter binds to the container's own loopback and is unreachable.
4. **Docker Desktop must be running**, ideally with the WSL2 backend.
5. **Paths:** PowerShell `${PWD}`; Git Bash needs `MSYS_NO_PATHCONV=1` (or
   `//workspace/adapt`); WSL2 is clean.
6. **Line endings:** a clone with `core.autocrlf=true` can give the `.sh` scripts
   CRLF and `bash\r: command not found`. The `docker` commands avoid this.

---

## TransAxx — building the upstream repo directly

**GPU-only and much heavier** (a multi-GB CUDA image; needs an NVIDIA GPU,
matching CUDA, and the nvidia-container-toolkit). I did **not** rebuild it from
scratch — unlike AdaPT, upstream's TransAxx is mostly version-pinned and uses a
prebuilt PyTorch base image, so it likely still builds. The point of this repo's
TransAxx image is **not** to fix a broken build; it's:

- **Version fidelity to the paper.** Upstream's Dockerfile is `FROM
  pytorch/pytorch:2.0.1-cuda11.7` (torch 2.0.1). The TransAxx paper states it was
  evaluated on **torch 1.13 / CUDA 11.7**, so this repo builds on
  `nvidia/cuda:11.7.1` + `torch==1.13.1+cu117` to reproduce the paper's setup.
- **Build-tool fallout of torch 1.13** — `setuptools<70`, `pip<24`, and the
  editable `--no-build-isolation` install of the vendored `pytorch-quantization`
  (see "why those pins exist" above).
- **The same wrapping** as AdaPT: Jupyter Lab auto-start, bind-mounted
  notebooks/results, LUT dir mounted over TransAxx's JIT-kernel path, and a CDI
  compose override for NixOS/Podman GPU hosts.

If you want the original upstream TransAxx image instead (torch 2.0.1), its own
`docker/build_docker.sh` should still work on a CUDA host — but its results are
on a different PyTorch than the paper, and it has none of the reproducibility
wrapping above. Use this repo's `compose up transaxx` instead.
