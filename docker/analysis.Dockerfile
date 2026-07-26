# builds the research project (rp) analysis image. lightweight, no framework, for the
# analysis notebooks (0_build_multipliers, 0_configure_sweeps, 2_accuracy_comparison,
# 3_multiplier_deepdive, 4_multiplier_metrics, 5_arch_comparison).
# no torch, no cuda; they read results/runs.csv and the multiplier sources (notebook 0
# also writes the LUT headers + manifest, still pure python). kept separate so you dont
# boot a 16gb cuda image just to draw a chart or recompute the NAME table, and so one
# container sees both frameworks results at once for the adapt-vs-transaxx comparison.
#
#   docker compose -f docker/docker-compose.yml up analysis   # -> http://localhost:8890
#
# build context is the repo root:
#   docker build -t rp-analysis -f docker/analysis.Dockerfile .

FROM python:3.11-slim

ENV DEBIAN_FRONTEND=noninteractive \
    PIP_NO_CACHE_DIR=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# gcc/make to run the C generator at build time (below), which bakes the reference
# LUTs that 0_build_multipliers cross-checks its Python output against. bash because
# build_all.sh is a bash script. The notebooks themselves are pure python.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gcc make bash ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# pip dependencies first so a multiplier/notebook change does not retrigger this layer
COPY requirements-analysis.txt /tmp/requirements-analysis.txt
RUN python3 -m pip install --upgrade pip && \
    python3 -m pip install -r /tmp/requirements-analysis.txt

# Bake the multiplier sources + generated LUT headers so notebooks 0 and 4 run fully
# offline (0's cross-check and 4's LUT metrics read luts/<fw>/*.h).
COPY multipliers/ /workspace/multipliers/
RUN cd /workspace/multipliers && bash generate/build_all.sh

# Jupyter roots at /work; compose bind-mounts ./notebooks -> /work/notebooks and
# ./results -> /work/results here too.
WORKDIR /work
EXPOSE 8890

CMD ["jupyter", "lab", \
     "--ip=0.0.0.0", "--port=8890", "--no-browser", "--allow-root", \
     "--NotebookApp.token=", "--NotebookApp.password="]
