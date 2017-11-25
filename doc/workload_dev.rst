Adding a new workload
=====================

This document explains how to create a new workload and the functions available
to the workload implementation.

Defining a new workload
-------------------------

1. Create a pair of `.c` and `.h` file in `src/client/workloads/`

2. Define functions for the operations defined in
   :c:type:`workload_functions_t` in the `.c` file:

    * :c:member:`workload_functions_t.on_init`
    * :c:member:`workload_functions_t.on_tick`
    * :c:member:`workload_functions_t.on_get_response`
    * :c:member:`workload_functions_t.on_put_response`
    * :c:member:`workload_functions_t.on_rotx_response`

    It is recommended to leave those functions empty for now, indications for
    implementing them are given in due time bellow.

3. Define a structure of type :c:type`workload_functions_t` in the `.c` file
   and fill it with the previously defined functions. For instance:

    .. code-block:: c

        workload_functions_t workload_example = {
            .on_init = my_on_init,
            .on_tick = my_on_tick,
            .on_get_response = my_on_get_response,
            .on_put_response = my_on_put_response,
            .on_rotx_response = my_on_rotx_response
        };

4. Declare this structure in the `.h` file. It must be declared as `extern`
   when included from the associated `.c` file and not `extern` otherwise.
   Here's an example of such `.h` file, assuming
   ``#define client_workload_example_c`` is present in the `.c` file:

    .. code-block:: c

       #ifndef client_workload_example_h
       #define client_workload_example_h

       #include "client/workloads.h"

       #if defined client_workload_example_c
       #define SCLASS extern
       #else
       #define SCLASS
       #endif
       SCLASS workload_functions_t workload_example;
       #undef SCLASS

       #endif

5. Add an ``#include`` of the new `.h` file and a definition of the new
   workload to `src/client/workloads.c`.

6. Run ``make`` to verify that the code compiles.

Once the code added code compiles without error and warnings, you may continue by implementing the empty functions with the help of the next section.


Implementing a new workload
---------------------------

A few functions are available to the workload implementation, those are the
functions that would be provided by the client API of the synchronisation
protocol:

* :c:func:`client_get_request`
* :c:func:`client_put_request`
* :c:func:`client_rotx_request`
* :c:func:`client_schedule_tick`
* :c:func:`client_last_request_duration`
* :c:func:`client_thinking_time`

.. todo::

    Protocols may define additional funcions


Initialization
""""""""""""""

During client initialization the :c:member:`workload_functions_t.on_init`
functions is called. This function provided by the workload should initialize
the state of the workload. To this end the workload assigns the pointer pointed
to by the ``data`` argument. This pointer will be given again in all calls to
workload functions (:c:type:`workload_functions_t`).

For instance, to set the state to an int:

.. code-block:: c

    void on_init (void **data, client_state_t *cs, cluster_config_t *cluster)
    {
        // Allocate the state
        int *int_ptr = malloc(sizeof(int));

        // Initialize the state
        *int_ptr = 42;

        // Set the state
        *data = int_ptr;

        // Perform a request
        ...
    }

To complete this example here is how this state would be accessed in the other functions:

.. code-block:: c

    void on_get_response(void **data, client_state_t *cs, gr_value value)
    {
        int *int_ptr = (int *)(*data);
    }

A real implementation of a workload would likely use a `struct` instead of an
`int` but will proceed in exactly the same way.


Delaying actions
""""""""""""""""

The workload uses the :c:func:`client_schedule_tick` function to delay its
actions. A call to that functions schedules a future call to the
:c:member:`workload_functions_t.on_tick` function. For instance a workloads
that want to perform a new get request 15 milliseconds after receiving a get
response could use the following set of functions:

.. code-block:: c

    void on_get_response(void **data, client_state_t *cs, gr_value value)
    {
        client_schedule_tick(cs, 15e-3);
    }

    void on_tick(void **data, client_state_t *cs)
    {
        gr_key key = 1;
        client_get_request(cs, key);
    }


client/workloads.h
------------------

.. include-comment:: ../src/client/workloads.h


client/workloads.c
------------------

.. include-comment:: ../src/client/workloads.c
