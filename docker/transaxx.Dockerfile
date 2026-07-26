# builds the research project (rp) transaxx image. GPU counterpart to rp-adapt, runs the
# same inference + LUT setup through TransAxx instead.
# 
# build context is the repo root:
#   docker build -t rp-transaxx -f docker/transaxx.Dockerfile .
#   docker run --gpus all --rm -p 8889:8889 rp-transaxx
#
# host needs an NVIDIA driver and the nvidia-container-toolkit. on a CDI host
# (nixos, podman, recent docker/fedora) the toolkit hands out GPUs over the
# Container Device Interface rather than the legacy nvidia runtime, so chain on
# the cdi override (docker-compose.cdi.yml).
 
FROM nvidia/cuda:11.7.1-cudnn8-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8 \
    PIP_NO_CACHE_DIR=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    CUDA_HOME=/usr/local/cuda \
    FORCE_CUDA=1

# ubuntu 22.04 ships gcc 11, which is the newest CUDA 11.7 accepts.
# dont apt-get install gcc-12 or anything that would shift the default.
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3 python3-pip python3-venv python3-dev \
        build-essential gcc g++ make git curl ca-certificates \
        locales \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen \
    && rm -rf /var/lib/apt/lists/*

# pip dependencies
COPY requirements-transaxx.txt /tmp/requirements-transaxx.txt
RUN python3 -m pip install --upgrade "pip<24" && \
    python3 -m pip install -r /tmp/requirements-transaxx.txt

# clone TransAxx at /workspace like the original code expects
ARG TRANSAXX_REF=main
RUN git clone https://github.com/dimdano/transaxx.git /workspace && \
    cd /workspace && git checkout ${TRANSAXX_REF}

ENV PYTHONPATH=/workspace

# install the vendored pytorch-quantization (NOT the pypi one, TransAxxs
# layers depend on this exact fork). --no-build-isolation because its
# setup.py imports torch at build time, which fails in pips isolated env.
RUN cd /workspace/pytorch-quantization && \
    python3 -m pip install --no-build-isolation -e .

# generator + LUT sources. build_all.sh writes the canonical C-generated headers to
# /workspace/multipliers/luts/, the baked reference for 0_build_multipliers' cross-check
# and notebook 4's LUT metrics. They are NOT copied into the JIT kernel dir: at run time
# docker-compose bind-mounts ./luts/transaxx OVER ext_modules/include/nn/cuda/axx_mults/
# (TransAxx's JIT-compile lookup path), so notebook 0's headers are the single source of
# truth and a new multiplier needs no rebuild.
COPY multipliers/ /workspace/multipliers/
RUN cd /workspace/multipliers && bash generate/build_all.sh

# notebooks are not baked into the image. docker-compose bind-mounts ./notebooks
# -> /work/notebooks so you can edit params and re-run without a rebuild, and theres
# one copy of each notebook instead of a baked one per image. jupyter roots at /work,
# where compose also mounts ./results -> /work/results. TransAxx is on PYTHONPATH
# (/workspace), so `from classification...` works from any directory.
WORKDIR /work
EXPOSE 8889

# default command to start jupyter lab, for local use only.
CMD ["jupyter", "lab", \
     "--ip=0.0.0.0", "--port=8889", "--no-browser", "--allow-root", \
     "--NotebookApp.token=", "--NotebookApp.password="]
