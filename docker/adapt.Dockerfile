# builds the research project (rp) adapt image. CPU side of the pipeline, runs inference
# through AdaPT with our approximate multiplier LUTs.
# 
# build context is the repo root, not docker/. run from there:
#   docker build -t rp-adapt -f docker/adapt.Dockerfile .
#   docker run --rm -p 8888:8888 rp-adapt
# easier path is docker compose, see docker/docker-compose.yml.

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8 \
    PIP_NO_CACHE_DIR=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# basic dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3 python3-pip python3-venv python3-dev \
        build-essential gcc g++ make git curl ca-certificates \
        locales \
    && sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen \
    && rm -rf /var/lib/apt/lists/*

# pip dependencies first such that a code/notebook change does not retrigger this layer
COPY requirements-adapt.txt /tmp/requirements-adapt.txt
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install -r /tmp/requirements-adapt.txt

# for some reason AdaPT hardcodes the path /workspace/adapt so we mirror that
# pin to a specific commit with --build-arg ADAPT_REF=<sha> for reproducibility
ARG ADAPT_REF=main
RUN git clone https://github.com/dimdano/adapt.git /workspace/adapt && \
    cd /workspace/adapt && git checkout ${ADAPT_REF}

# notebook imports like `from adapt.approx_layers import ...` need the
# adapt repo root on sys.path. upstream sets this in bashrc, but jupyter
# kernels dont read bashrc, so we set it as an image level env so the kernel inherits it.
ENV PYTHONPATH=/workspace/adapt

# bring in the generator and multiplier sources. build_all.sh writes the canonical
# C-generated headers to /workspace/multipliers/luts/{adapt,transaxx}/, kept as the
# baked reference that 0_build_multipliers' cross-check validates against (and notebook 4
# reads for LUT metrics). They are NOT copied into the JIT kernel dir: at run time
# docker-compose bind-mounts ./luts/adapt OVER axx_mults, so the headers from notebook 0
# are the single source of truth and a new multiplier needs no rebuild.
COPY multipliers/ /workspace/multipliers/
RUN cd /workspace/multipliers && bash generate/build_all.sh

# notebooks are not baked into the image. docker-compose bind-mounts ./notebooks
# -> /work/notebooks so you can edit params and re-run without a rebuild, and theres
# one copy of each notebook instead of a baked one per image. jupyter roots at /work,
# where compose also mounts ./results -> /work/results. the AdaPT package is on
# PYTHONPATH (/workspace/adapt), so imports work from any working directory.
WORKDIR /work
EXPOSE 8888

# default command to start jupyter lab, for local use only.
CMD ["jupyter", "lab", \
     "--ip=0.0.0.0", "--port=8888", "--no-browser", "--allow-root", \
     "--NotebookApp.token=", "--NotebookApp.password="]
