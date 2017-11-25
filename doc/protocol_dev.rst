Adding a new protocol
=====================

To add a new protocol there are three separate tasks to do:

1. Add the :ref:`protocol_message_definitions`
2. Add the :ref:`protocol_server_implementation`
3. Add the :ref:`protocol_client_implementation`


.. _protocol_message_definitions:

Message definitions
-------------------

Define the network messages used by the new protocol in a `.h` file in the
`src/messages` directory. Each message must be a struct starting with the macro
`MESSAGE_STRUCT_START` which will add the fields common to all macro and allows
the new messages type to safely be used as a `message_t`.

Here's an example of a simple message definition:

.. code-block:: c

    #include "messages/message.h"

    typedef struct new_protocol_get_request {
        MESSAGE_STRUCT_START;
        gr_key key;
    } new_protocol_get_request_t;

.. _protocol_server_implementation:

It is recommended to define a function to instantiate the messages. This can
conveniently be achieved from a `.c` file for your protocol and the macros
defined in `src/messages/macros.h`. Look at `src/messages/gr.h` and
`src/messages/gr.c` for a complete example.

Server implementation
---------------------

To add a new protocol to the server create a folder with your protocol name in
`src/server/protocols/`. The implementation can be split across several files
in that folder.

The server implementation of the protocol must provide the 3 functions of the
:c:type:`server_functions_t` structure. How to do this is explained in the
subsections bellow.

To make the protocol available in the simulation it is needed to define a
:c:type:`server_functions_t` structure and to add it to
:c:data:`protocol_definitions` in `src/protocols.c`. In order to ensure the
structure is defined only once for your protocol it must be declared as
`extern` when included from the `.c` file and not `extern` otherwise.

Here's an example of how this would look like:


* `src/server/protocols/example/example.c`

  .. code-block:: c

      #define server_protocols_example_example_c
      #include "server/protocols/example/example.c"
      #include "server/server.h"

      static server_state_t *example_allocate_state(void)
      {
      }

      static void example_init_state (server_state_t *server_state)
      {
      }

      static int
      example_process_event(server_state_t *server_state,
                            unsigned int envent_type,
                            void *data, size_t data_size)
      {
      }

      server_functions_t example_server_funcs = {
          .allocate_state = example_allocate_state,
          .init_state = example_init_state,
          .process_event = example_process_event
      };

* `src/server/protocols/example/example.h`

  .. code-block:: c

      #ifndef server_protocols_example_example_h
      #define server_protocols_example_example_h

      #ifdef server_protocols_example_example_c
      #define SCLASS
      #else
      #define SCLASS extern const
      #endif
      server_functions_t example_server_funcs;
      #undef SCLASS

      #endif


State definition
^^^^^^^^^^^^^^^^

The state of the server (:c:type:`server_state_t`) must be extended to include
the state of your protocol.  The easiest way of doing it that doesn't require
casting and still allow easy access to the members of :c:type:`server_state_t`
is as shown bellow (this definition is usually most useful in the previously
created `.h` file):

.. code-block:: c

    typedef struct {
        union { // Must be the first element of the struct
            SERVER_STATE_STRUCT();
            server_state_t server_state;
        };
        // Add members as needed here
    } example_server_state_t;

With this definition a pointer to :c:type:`server_state_t` required by the
functions presented bellow is obtained with
``&example_server_state_ptr->server_state``. Members of
:c:type:`server_state_t` are directly available with
``example_server_state_ptr->member``.

.. important::

    A pointer to the `server_state_t server_state` member of this struct MUST
    point to the beginning of the struct. Thus, as shown in the example, this
    member MUST be the first element of the struct.


.. _protocol_server_initialization:

Initialization
^^^^^^^^^^^^^^

At initialization the server first calls
:c:member:`server_functions_t.allocate_state`. This function is responsible for
allocating a memory region to store the state. It will typically look like this:

.. code-block:: c

    static server_state_t *example_allocate_state(void)
    {
        example_server_state_t *example = malloc(sizeof(example_server_state_t));
        return &example->server_state;
    }

The server then calls :c:member:`server_functions_t.init_state`. This function
is called after :c:type:`server_state_t` is initialized and is responsible for
the initialization of the state specific to the protocol. It will look like this:

.. code-block:: c

    static void example_init_state(server_state_t *server_state)
    {
        example_server_state_t *state = (example_server_state_t *) server_state;
        // Initialize the protocol-specific state
    }

Receiving messages
^^^^^^^^^^^^^^^^^^

On reception of a message, the function
:c:member:`server_functions_t.process_event` is called. This function must
return 1 if it handled the event, 0 otherwise. To keep tho code easy to read it
is best to use this function to dispatch to event-specific functions as shown
bellow:

.. code-block:: c

    static void example_process_event(server_state *server_state, unsigned int event_type, void *data, size_t data_size)
    {
        example_server_state_t *state = (example_server_state_t *) server_state;
        switch (event_type) {
            case EXAMPLE_MESSAGE:
                on_example_message(state, data);
                break;
            default:
                return 0;
        }
        return 1;
    }

Sending messages
^^^^^^^^^^^^^^^^

To send a message to a client or another server, :c:func:`server_send` is used.
This functions will takes care of accounting of the time needed to send the
message and ensure the event is delivered to the destination at the correct
time.


Accumulating execution time
^^^^^^^^^^^^^^^^^^^^^^^^^^^

For each event it handles, the protocol implementation is responsible for
accumulating how much CPU time is used. This is done by calling
:c:func:`cpu_add_time`. This function can be called several time during event
processing. To compute the time spent, the timing parameters specified in the
configuration file should be used.

Defining timing parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

To define a timing parameters, the :c:macro:`DEFINE_PROTOCOL_TIMING_FUNC` is used. It is used like this:

.. code-block:: c

    DEFINE_PROTOCOL_TIMING_FUNC(example_message_time, "example")

    void on_example_message(example_server_state_t *state, example_message_t *message)
    {
        cpu_add_time(state->cpu, example_message_time());
    }

Which corresponds to this parameter in the configuration file::

    {
        "timing": {
            "example": {
                "example_message_time": 1e-3;
            }
        }
    }

Each time the function defined with :c:macro:`DEFINE_PROTOCOL_TIMING_FUNC` is
called a new value is returned according to the configured timing distribution
(see `common_timing_parameters`).

Statistics
^^^^^^^^^^

The protocol implementation is responsible for calling functions to compute
statistics. At the very least it must compute the following statistics:

* Number of get requests

  Call :c:func:`server_stats_counter_inc` with :c:data:`GET_REQUESTS` as second
  argument.

* Number of put requests

  Call :c:func:`server_stats_counter_inc` with :c:data:`PUT_REQUESTS` as second
  argument.

* Number of rotx requests

  Call :c:func:`server_stats_counter_inc` with :c:data:`ROTX_REQUESTS` as
  second argument.

* Number of updates from replica

  Call :c:func:`server_stats_counter_inc` with :c:data:`REPLICA_UPDATES` as
  second argument.

* Replication time

  Compute the replication time of each value received from another replica and call:
  :c:func:`server_stats_array_push` with :c:data:`REPLICATION_TIME` as second argument.

* Value staleness

  Compute the staleness of each value returned to a client and call:
  :c:func:`server_stats_array_push` with :c:data:`VALUE_STALENESS` as second argument.

* Visibility latency

  Whenever a value becomes visible, compute its visibility latency and call:
  :c:func:`server_stats_array_push` with :c:data:`VISIBILITY_LATENCY` as second argument.

The protocol implementation may also compute other statistics by first
declaring them with :c:func:`server_stats_counter_new` or
:c:func:`server_stats_array_new` and using the returned id with either
:c:func:`server_stats_counter_inc` or :c:func:`server_stats_array_push`.




.. _protocol_client_implementation:

Client implementation
---------------------

The client implementation is organized in a way similar to the
:ref:`protocol_server_implementation`. The files are to be created in
`src/client/protocols/`.

The protocol implementation must define a variable of time
:c:type:`client_functions_t` and the relevant entry in
:c:data:`protocol_definitions` (in `src/protocols.c`) must point to it.

A minimal example follows, see the later subsections for details on the
individual functions.

.. code-block:: c

    #define client_protocols_example_c
    #include "client/protocols/example.c"
    #include "client/client.h"

    typedef struct {
        const client_config_t *config;
    } client_state_example_t;

    static void *init(client_state_t *client,
                      const client_config_t *config)
    {
        client_state_example_t *state =
                malloc(sizeof(client_state_example_t));
        state->config = config;
        return state;
    }

    static void get_request(client_state_t *client, gr_key key)
    {
        client_state_example_t *state = client_protocol_state(client);
    }

    static void put_request(client_state_t *clietn, gr_key key,
                            gr_value value)
    {
        client_state_example_t *state = client_protocol_state(client);
    }

    static void rotx_request(client_state_t *client, gr_key *keys,
                             unsigned int num_keys)
    {
        client_state_example_t *state = client_protocol_state(client);
    }

    static void process_event(client_state_t *client, simtime_t now,
                              unsigned int event_type, void *data,
                              size_t data_size)
    {
        client_state_example_t *state = client_protocol_state(client);
        switch (event_type) {
            default:
                return 0;
        }
        return 1;
    }

    client_functions_t example_client_funcs = {
        .init = init,
        .get_request = get_request,
        .put_request = put_request,
        .rotx_request = rotx_request,
        .process_event = process_event
    };

And the associated `.h` file:

.. code-block:: c

    #ifndef client_protocols_example_h
    #define client_protocols_example_h

    #include "protocols.h"

    #ifdef client_protocols_example_c
    #define SCLASS
    #else
    #define SCLASS extern const
    #endif
    client_functions_t example_client_funcs;
    #undef SCLASS

    #endif


Initialization
^^^^^^^^^^^^^^

When the client is initialized, the function
:c:member:`client_functions_t.init` is called. This function must allocate and
initialize the state of the protocol and return a pointer to it.

Receiving messages
^^^^^^^^^^^^^^^^^^

Upon reception of a message from the network (or another event), the
:c:member:`client_functions_t.process_event` function is called. It must return
1 if the event has been handled an 0 otherwise.

To deliver the message to the workload, call the relevant function of the
:c:type:`workload_functions_t` structure. This structure is accessible in the
:c:type:`client_config_t` structure. The client implementation gets a pointer
to that structure when :c:member:`client_functions_t.init` is called.

The pointer to the workload state is obtained by a call to
:c:func:`client_workload_state_ptr`.

Sending messages
^^^^^^^^^^^^^^^^

The client sends network messages using the :c:func:`client_send` function.

Statistics
^^^^^^^^^^

In order for statistics to be computed, the implementation must call
:c:func:`client_begin_request` when it starts a new request and
:c:func:`client_finish_request` when it gets the response from the server.
Request types are created using the :c:func:`client_register_request_type`
function which is best called from :c:member:`client_functions_t.init`.
