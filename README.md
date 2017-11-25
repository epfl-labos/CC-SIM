# Install

1. Install ROOT-Sim with the patch to run code before the simulation starts.
2. Install libjson-c >= 0.12
3. Compile the simulator by running make in this directory

## Sample installation on Debian

This doesn't cover the installation of the compiler toolchain and the header
files of librairies not used directly by CC-Sim. However you can install them
as usual with aptitude/apt-get as you discover them missing when following the
steps bellow.

###1. Install ROOT-Sim

This example uses GNU Stow but you can also install ROOT-Sim directly
in /usr/local if you are comfortable doing so.

    cd somewhere
    git clone "ssh://git@c4science.ch/diffusion/3634/root-sim.git" root-sim
    cd root-sim
    ./configure --prefix=/usr/local/stow/root-sim
    sudo mkdir -p /usr/local/stow/root-sim
    make
    sudo make install
    cd /usr/local/stow
    sudo stow --stow root-sim

###2. Install libjson-c >= 0.12

On Debian Stretch, simply install the `libjson-c-dev` package.

Debian Jessie has only version 0.11 but 0.12 can be easily backported.

    cd somewhere
    apt-get source -t unstable libjson-c3
    aptitude build-dep libjson-c3
    cd json-c-0.12.1
    dpkg-buildpackage -uc -us
    cd ..
    sudo dpkg -i *.deb

###3. Compile the simulator by running make in this directory.

# Usage

Sample usage: Simulate 1 second using the sequential implementation of
ROOT-Sim. Then gather statistics.

    rm -rf stats
    make run RUN_ARGS="--config config.json --stop-after=1 --output-dir=stats" ROOTSIM_ARGS="--sequential"
    ./experiments/stats.pl ./stats

Run the last simulation again inside GDB using the same RNG seed as the last run.

    make gdb RUN_ARGS="--config config.json --stop-after=1 --output-dir=stats" ROOTSIM_ARGS="--sequential --deterministic_seed"

# Documentation

To generate the documentation, run `make doc` you'll need `python3` and
`python-virtualenv` installed on your system. Once the documentation is
successfully generated the URL of the index is printed on the terminal.

#Contributors
Florian Vessaz (florian@florv.ch)

# Contacts
Florian Vessaz (florian@florv.ch)
Diego Didona (diego.didona@epfl.ch)
