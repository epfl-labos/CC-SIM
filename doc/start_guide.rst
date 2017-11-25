Getting started
===============


Dependencies
------------

* `GCC <https://gcc.gnu.org/>`_
* `GNU Make <https://www.gnu.org/software/make/manual/make.html>`_
* `ROOT-Sim <https://github.com/HPDCS/ROOT-Sim>`_
* `json-c <https://github.com/json-c/json-c/wiki>`_ ≥ 0.12


GNU Make and GCC
""""""""""""""""

CC-Sim is compiled with GNU Make and GCC. Use GCC version 6 or a later stable
version. On Debian Stretch and later they can simply be installed using
`aptitude install gcc make`.


json-c
""""""

On Debian Stretch the json-c library can be installed with ``aptitude install
libjson-c-doc``.

On older releases it has to be backported manually here's a manner to do it::

    cd somewhere
    apt-get source -t unstable libjson-c3
    aptitude build-dep libjson-c3
    cd json-c-0.12.1
    dpkg-buildpackage -uc -us
    cd ..
    sudo dpkg -i *.deb


ROOT-Sim
""""""""

CC-Sim currently depends on a slightly modified version of ROOT-Sim that allows
running code before the simulation is started. This version of ROOT-Sim is
available from ssh://git@c4science.ch/diffusion/3634/root-sim. See ROOT-Sim's
README for instructions on how to compile it.

Here's a sample installation transcript using
`Stow <https://www.gnu.org/software/stow/>`_ and Debian Stretch::

    cd somewhere
    git clone "ssh://git@c4science.ch/diffusion/3634/root-sim.git" root-sim
    cd root-sim
    sudo aptitude install aclocal libtool
    ./autogen.sh
    ./configure --prefix=/usr/local/stow/root-sim
    sudo mkdir -p /usr/local/stow/root-sim
    make
    sudo make install
    cd /usr/local/stow
    sudo stow --stow root-sim


Compiling
---------

Once the dependencies are installed, CC-Sim can compiled using ``make``.


Running
-------

The general usage is ``./application cc_sim_arguments -- root_sim_arguments``
where ``cc_sim_arguments`` are the arguments documented bellow and
``root_sim_arguments`` are the one found in the ROOT-Sim documentation.

Example invocation
""""""""""""""""""

``./application --config config.json --stop-after=1 --output-dir=stats -- --sequential``

This runs the simulation with the parameters specified in ``config.json``,
stops the simulation after 1 second of simulated time and puts statistics in the
``stats`` directory. The simulation is executed with the sequential
implementation of ROOT-Sim.

.. note::

    Currently only the sequential implementation of ROOT-Sim is supported. Thus
    ``--sequential`` must always be provided in the ROOT-Sim arguments for a
    correct simulation.

List of command-line arguments
""""""""""""""""""""""""""""""

``--config file``
    Indicate which configuration file to use (mandatory).

``--stop-after seconds``
    Stop the simulation after the specified number of simulated seconds.

``--stop-after-real-time seconds``
    Stop the simulation after the specified number of real time seconds.

``--output-dir directory``
    Write statistics in the specified directory.


Configuration
-------------

The configuration file is in the `JSON file format <http://json.org/>`_. An
example configuration file (`config.json`) is provided with CC-Sim.

* All floating point values can be defined in scientific notation as in `12.3e-4`.
* To avoid confusion, all time values are given in seconds.

The configuration file is split into several objects:

`application`
    Parameters defining of the application executes the simulation.

`cluster`
    Parameters defining the configuration of the simulated cluster.

`timing`
    Timing parameters used to determine the time needed for each operation.

`protocol`
    Parameters of synchronization protocols.

`network`
    Parameters describing the network configuration.

`client`
    Parameters describing the configuration of the clients.

`workload`
    Workload-specific parameters.


"application" parameters
""""""""""""""""""""""""

`protocol`
    The name of the protocol to execute. See the :ref:`protocols` section for
    possible values.

`stop_after_simulated_seconds`
    Stop the simulation after a certain amount of simulated seconds. A floating
    point value can be specified.

`stop_after_real_time_seconds`
    Stop the simulation after a certain amount of real time seconds. A floating
    point value can be specified.

`ignore_initial_seconds`
    Ignore the initial seconds of the simulation when computing statistics. A
    floating point value can be specified.

"cluster" parameters
""""""""""""""""""""

`clients_per_partition`
    The number of client per partitions.

`keys`
    The number of keys in the data store.

`partitions_per_replica`
    The number of partitions per replica (number of servers per data center).

`replicas`
    The number of replicas (data centers).

.. _common_timing_parameters:

Common timing parameters
""""""""""""""""""""""""

The ``"timing"`` object contains timing parameters common to all protocols as
well as on object per protocol with protocol-specific parameters (see
:ref:`protocols`). Only the common parameters and the ones for the selected
protocol may be required.

`build_struct_per_byte_time`
    Average time to allocate and fill one byte of memory.

.. _timing_client_thinking_time:

`client_thinking_time`
    Time needed for a client to send a new request after a response from a server.

`server_send_per_byte_time`
    Average time needed by a server to send one byte of data on the network.

`server_send_time`
    Additional time needed by a server to perform any request to send data on
    the network.

`timing_distribution`
    The timing distribution used when drawing value for any timing parameter.
    See the :ref:`timing_distributions` section bellow for possible values.

`lock_time`
    The time needed to acquire or release a lock.

"protocol" parameters
"""""""""""""""""""""

This object optionally contains an object for each protocol with
protocol-specific parameters. See the :ref:`protocols` section for what those
parameters are. Only the section for the simulated protocol may be required.

"network" parameters
""""""""""""""""""""

`intra_datacenter_delay`
    The propagation delay in seconds of a message between two server of a data center.

`inter_datacenter_delay`
    A matrix used to determine the propagation delay in second of a message
    between two servers of different data centers. The line indicate the source
    and the row the destination.

`self_delay`
    The delay in seconds for messages send to the sender.

`transmission_rate`
    The transmission rate in bit/s of the network adapters of clients and servers.

"client" parameters
"""""""""""""""""""

`workload`
    The workload executed by the clients. See the :ref:`workloads` section for
    possible values.

`key distribution`
    The distribution to use for determining the popularity of keys currently
    `uniform` and `zipfian` are implemented.

`zipfian skew`
    The skew parameter of the Zipfian distribution.

"workload" parameters
"""""""""""""""""""""

This object contains on object for each workload with workload-specific
parameters See the :ref:`workloads` section for what those parameters are. Only
the section for the selected workload may be required.

.. _timing_distributions:

Timing distributions
""""""""""""""""""""

The timing distribution is used to draw a value each time a timing parameters
is used. This allows for non-constant timing parameters. The following
distribution are currently available:

`constant`
    Always use the values defined in the configuration file.

`exponential`
    Draw values according to the exponential distribution.

`normal`
    Draw values according to the normal distribution. Additionally the timing
    parameters `normal_mu` and `normal_sigma` must be provided.
