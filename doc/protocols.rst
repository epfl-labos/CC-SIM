.. _protocols:

Synchronization protocols
=========================

CC-Sim currently implements several synchronization protocols:

.. _protocols_gentlerain:

GentleRain
-----------

GentleRain is implemented as described in `GentleRain: Cheap and Scalable
Causal Consistency with Physical Clocks <https://infoscience.epfl.ch/record/202079>`_.
In CC-Sim, this algorithm is named ``gr``.

This protocol requires two parameters:

`clock_interval`
    The interval in seconds between two heartbeat message.

`gst_interver`
    The interval in seconds between two computation of the GST.

Here's an excerpt of a configuration file for using GentleRain::

    {
        "application": {
            "protocol": "gr"
        },
        "protocol": {
            "gr": {
                "clock_interval": 0.01,
                "gst_interval": 0.005
            }
        }
    }

.. _protocols_gentlerain_timing:

Timing parameters
"""""""""""""""""

For GentleRain, in addition to the :ref:`common_timing_parameters` the following additional ones are needed (as for
other parameters of CC-Sim, the time unit is the second and floating point
values can be provided in scientific notation):

`check_gst_time`
    The time needed to check whether the GST needs to be updated.

`get_value_time`
    The time to process a get request locally.

`is_value_visible_time`
    The time to determine whether a particular value is visible.

`min_replica_version_per_replica_time`
    The time to compute the minimum of the version vector for one replica.

`process_clock_tick_time`
    The time to process a clock tick.

`process_get_request_pre_time`
    A time spent when processing any get request, regardless of whether it is
    forwarded or processed locally.

`process_heartbeat_time`
    The time to process an heartbeat message.

`process_lst_from_leaf_end_per_replica_time`
    The additional time needed for processing of the last LST message from a
    child during a GST computation round.

`process_lst_from_leaf_per_replica_time`
    The time spent when processing a LST message from a child.

`process_put_request_pre_time`
    A time spent when processing any put request, regardless of whether it is
    forwarded or processed locally.

`process_replica_update_time`
    The time spent when processing an update from a replica.

`process_slice_response_per_value_time`
    The time spent when processing a response to a slice request.

`put_value_time`
    The time spent to process a put request locally.

`update_gst_per_rotx_time`
    The time needed to update the state of a ROTX request when the GST is updated.

`update_gst_time`
    The time needed to update the GST.

Here's an excerpt of a configuration file defining those parameters::

    {
        "timing": {
            "gr": {
                "check_gst_time": 1e-08,
                "get_value_time": 29.87e-09,
                "is_value_visible_time": 30.74e-09,
                "min_replica_version_per_replica_time": 1.05e-06,
                "process_clock_tick_time": 2.04e-06,
                "process_get_request_pre_time": 300e-09,
                "process_heartbeat_time": 2e-06,
                "process_lst_from_leaf_end_per_replica_time": 29.9e-08,
                "process_lst_from_leaf_per_replica_time": 0.5e-06,
                "process_put_request_pre_time": 12e-06,
                "process_replica_update_time": 3.12e-06,
                "process_slice_response_per_value_time": 10.5e-06,
                "put_value_time": 29.25e-09,
                "update_gst_per_rotx_time": 1.05e-05,
                "update_gst_time": 297e-09
            }
        },
    }


GentleRain Vector
------------------

GentleRain Vector is a variation of GentleRain using vector clock this is the
same technique used in `Cure <https://hal.inria.fr/hal-01350558>`_. In CC-Sim
this algorithm is named ``grv``.

This protocol requires the parameters of :ref:`protocols_gentlerain` to also be
present in the configuration file. Here's an excerpt of a configuration file
using GentleRain Vector::

    {
        "application": {
            "protocol": "grv"
        },
        "protocol": {
            "gr": {
                "clock_interval": 0.01,
                "gst_interval": 0.005
            }
        }
    }

Timing parameters
"""""""""""""""""

GentleRain Vector needs the :ref:`protocols_gentlerain_timing` of GentleRain
and the following additional ones:

`check_gst_vector_per_replica_time`
    Time needed per replica to check whether the GSV needs to be updated.

`process_rotx_request_per_partition_time`
    Time needed to do partition-specific processing of a ROTX request.

`update_get_vector_per_replica_time`
    Time needed to update an entry of the GSV.

`update_gst_vector_per_update_time`
    Time needed to check whether an entry of the GSV must be updated.

Here's an except of a configuration file defining those parameters::

    {
        "timing": {
            "gr": {
                ...
            },
            "grv": {
                "check_gst_vector_per_replica_time": 1e-08,
                "process_rotx_request_per_partition_time": 10.5e-06,
                "update_gst_vector_per_replica_time": 3.15e-06,
                "update_gst_vector_per_update_time": 1.38e-06
            }
        },
    }
