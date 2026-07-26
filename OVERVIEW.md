# What this project is

A short, plain-English description of what lives in this repo and why. If you
want to clone it and run it, the [`README.md`](README.md) and
[`docker/README.md`](docker/README.md) are the places to go. This file is just
the story.

## The question

Multiplication is the expensive part of running a neural network. An *approximate*
multiplier is one that gives up exactness on purpose: it gets some products a
little wrong, and in exchange it is smaller and cheaper to build in hardware (on
an FPGA or an ASIC). The bet is that a deep network, which is noisy and redundant
anyway, will not really notice. The point of this project is to check that bet.

The multipliers under test are five 8-bit approximate designs, ISH-1 to ISH-5
(code name `FPGA_ISH_N`; the AMPEREISH family, which uses Internal Self-Healing
to cancel some of its own error). They come from the AMPEREISH paper (IEEE
Access, 2026) out of the CAES group at the University of Twente. That paper had only tried them on small networks, so
the obvious next question, and the one it names as future work, is what they cost
on the models people actually run. This project is that follow-up.

## How we test them

We do not have the FPGA silicon, so we emulate. Every 8-bit multiplier can be
written out in full as a 256×256 table: feed it the two input bytes, read off the
product it would have produced. A small C pipeline in `multipliers/` builds that
table for each of the five designs, by composing the multiplier up from 2-bit
primitives to 4-bit blocks to the full 8-bit unit, and writes it out as a lookup
table (a LUT). That C pipeline is the original work in the repo.

Two existing frameworks take it from there. The first one is **AdaPT**, which
runs the network on the CPU and swaps the multiplies inside the convolution layers
(where a CNN does most of its work) for a lookup in our table. The fully-connected
layers are left exact. The second one is **TransAxx**, which does the same on the
GPU for a transformer's linear layers (the QKV and MLP projections, where a ViT
does most of its work), and is fast enough for the larger ImageNet models.
Both of them quantise the network to int8 first (that is what makes an 8-bit
multiplier meaningful in the first place) and then run normal inference, just with
the approximate multiplier standing in for the exact one. Neither framework is
ours. They are cloned from upstream at build time and used as-is.

The network is **not** retrained for the approximate hardware. We take a model
that was trained normally and just evaluate it with the multiplier swapped in.
That is the honest "drop it in and see" number. (The frameworks'
approximation-aware retraining is also wired into the sweep notebooks for
anyone who wants to explore it.)

## What we evaluate

- **The CNNs.** How much top-1 accuracy do the five multipliers cost on
  ordinary convolutional networks (ResNet-20/56 and VGG-11 on CIFAR-10/100)? And
  how does that compare to multipliers from the public EvoApprox library, where
  the error characteristics are already known?
- **The ViTs.** Vision Transformers are a different kind of architecture, and
  they lean on big matrix multiplies the way a CNN leans on convolutions. Does that
  make them more fragile to an approximate multiplier than a CNN is? This is the
  question the project was set up to answer. We run ViT-S, DeiT-S, and ViT-B on
  ImageNet for it.
- **The early-exit CNN.** A three-exit BranchyAlexNet on CIFAR-10 tests whether
  the approximation disturbs entropy-gated early exiting. Its model and
  checkpoint are not distributed in this repo (not yet publishable), so this
  part is reported in the paper but not re-runnable from here;
  `docker/notebooks/_template_sweep.ipynb` is the template for sweeping such a
  model.

## What we found

The short version: the approximation costs almost no accuracy. Across all the CNN and ViT
runs the five multipliers stay within about a percentage point of the exact int8
baseline, with no retraining. ISH-1 (`FPGA_ISH_1`) is the one that stands out, because its
table is the least accurate of the five, and even it costs under a percentage
point. The early-exit run says the same thing: the entropy-gated exit behaviour
and its accuracy-compute trade-off survive the approximation essentially
unchanged.

A couple of things are worth saying alongside that. For the ViTs the accuracy you
lose is almost all from squeezing the model down to int8 in the first place, not
from the approximate multiplier on top of it. Because only the linear layers are
approximated there, with the attention matmuls left exact, the fair comparison is
layer-for-layer: those approximated layers in the ViTs lose no more accuracy than
the approximated convolutions in the CNNs, so the transformers turn out no more
fragile to the approximation than the CNNs are. And when we line our
multipliers up against the EvoApprox baselines at a matched error rate, the thing
that actually predicts the accuracy drop is not the size of the error but its
*bias*. The ISH designs are close to zero-mean, which is a large part of why
they cost so little.

## How the pieces fit together

```
multipliers/  (C)            docker/ notebooks                 the paper
  primitives -> 4-bit ->  ->  build the LUTs        -> luts/
  8-bit -> 256x256 LUT        choose what to sweep   -> a config file
                              run the sweeps         -> results CSVs
                              analysis notebooks     -> tables + figures  -> the paper
```

Data goes one way. The C pipeline produces the tables, the sweep notebooks
produce the raw results, and the analysis notebooks turn those into the exact
tables and figures the paper embeds. Nothing in the paper is typed in by hand.
Every number traces back to a notebook that produced it, so the whole chain can
be re-run.

The whole thing runs in Docker, as three services (one for AdaPT, one for
TransAxx, one for the analysis). That setup is itself part of the contribution:
the original paper left "try it on real models" as future work, and a
reproducible pipeline the next person can pick up and extend, rather than
rebuild, is what makes that practical. The `README.md` and `docker/README.md` cover how to
run it.
