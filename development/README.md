# The fast stream software design pattern

## Objective

Develop an API that provides the minimum number of interfaces needs to
make "streams" as we try to define here.  We wish to minimize the number
of interfaces that we expose to the filter module (filter, component,
block, whatever => filter) writer so that filters may be reused.  By not
letting the filter control so much the filter becomes more reusable.  So
we do not let the filter have any control over how it runs, we let the
question how the filter runs be decided by a level above, like the end
user, or a next level middle-ware, but at all costs, not the filter.  The
filter also does not manage the connecting and passing of data between
filters.  The filter only sets limits on connecting and passing of data
between filters.  This restrictive filter definition distinguishes CRTS
from all other "streams" APIs (application programming interfaces).  

## Fast stream requirements

We are interested in a software design pattern that exposes it's structure
to the application layer.

Fast stream requires:

- filter modules.

- Filter modules read zero to some finite number of inputs from other
  filter modules and write zero to some finite number of outputs to other
  filter modules.

- The modules connect in a I/O (input/output) directed graph.

- The directed graph is not cyclic.

- We desire input/output flow from sources to sinks be fast relative to
  the rate of change of control parameters.  TODO: define a measure.
  The stream flow rate is a least an order of magnitude faster than the
  rate of change of the control parameters.  The 2 limiting cases are:
    1. having no changing controls
    2. controls changing so fast that every stream "chunk" is changed
      (effected) by a control change.  If the control is changed any
      faster then there will be some control values that are not effecting
      the stream.  We are refering to interal state of control parameters
      that is never be seen by the stream.  In this fast control case the
      control may as well be a stream too, and we replace the control with
      a filter that merges this control-filter channel and the originial
      filter channel that we are controling, and now the control has flow
      (and my head hurts).  In this limiting case the control is not a
      control, but a flow that is interacting with the other flows in
      the filters.


- (TODO) Control parameters need to be able to be grouped in order to keep
  them consistent.  Like for example there may be a equation of constraint
  that needs to be applied to a group of parameters, like for example a
  geometric constraint like A^2 + B^2 = C^2, where A and B are
  parameters C is a value that we don't have the luxury knowing at the
  time of setting A and B.  Or a more general example: the domain of all
  valid parameters values just varies in a complex way that we may not
  have a model for (like someone else's code).



Fast stream API trends:

- Buffering between modules is automatically managed making it somewhat
  seamless.  Not all filters handle input and output data the same way.
  Different filters can require different framing schemes.  Ex: some
  require that there be only 12 bytes read at each input and others
  require the input be of size that is a multiple of 8 bytes, and others
  just don't care how many bytes come in.
    * simple buffering method
        - filters set threholds and limits on input chunks and frame the
          input data in the filter code.  Hence a stream user may make
          filter combinations that produce rubish.  There is no rules to
          how filters are connected.  CRTS does this.  
    * framing types method
        - filters configure "framing" which is a level above buffering
          that makes different types of filter connectors.  GNUradio does
          this.

- Filter modules register filter parameters that may be
  monitored and adjusted with "control" interfaces.

- Controllers can monitored and adjusted filter modules parameters
  via the filter modules controls.


Running streams:

- The processes that runs the filter modules should minimize the use
  of computer system resources in an automated/seamless fashion.

  Stream run states:

    * Off: there are no stream threads running and:
        - we are between filters start and stop,
        - filter start functions are being called, or
        - filter stop functions are being called;
        - this Off state is not seen by the filer input functions
          because they are not called in this state;
        - there is only the main thread running
        - Off sub-states:
            * initial load
                - filters are loaded connected parameters are
                  created
            * between stop and start
                - filters may be removed and reconnected
            * start
                - filters setup buffering based on connectivity
                - filters may allocate system resources if needed
            * stop
                - buffering is automatically freed
                - filters may clean up resources if needed
            * after stop, before exit
                - final report (??)
                - process exits and returns status

    * Running: filter input functions are being called.  A running flag is
      set and seen by the filter input functions.

    * Shutdown: a flag is unset and seen by the filter input functions,
      and the source filters input functions are called just once with
      the flag unset.  Non-source filters are called until they run out
      of data in the ring buffers.
        - in Shutdown mode there is a special filter input call called
          "lastCall" which is the last time the input will be called.
          This matters for filters that may recieve partial frames that
          will never be completed in the current stream run.  The filter
          needs to decide what to do with the incomplete data frame since
          that ring buffer will not preserve data between stop and the
          next start.  TODO: we need to add code to deal with this case.



## Software package examples


### GStreamer - https://gstreamer.freedesktop.org/

 For each incoming buffer, one buffer will go out, too.
 https://gstreamer.freedesktop.org/data/doc/gstreamer/head/manual/manual.pdf

### DirectShow - https://en.wikipedia.org/wiki/DirectShow

### GNUradio - https://www.gnuradio.org/

### REDHawk - https://redhawksdr.github.io/Documentation/index.html


## References

### SCA - https://en.wikipedia.org/wiki/Software_Communications_Architecture




## Nomenclature

### Varying terminology

Different software package frameworks have different terminology to refer
to different aspects of fast streams.  The class and variable naming
schemes vary across software packages.    



| Package        | stream | filter     |  input      |  output   | control | parameter | controller |
---------------------------------------------------------------------------
| GNURadio      |         |  Block      |             |           |            |
| GStreamer     |         |  Element    | source pad  | sink pad  |            |
| REDHawk       |         |  Component  |             |           |            |
| DirectShow    |         |             |             |           |            |

## Nomenclature


### Stream or Flow graph

### 

##

## The stream controller


## Examples

### Sound

### Sound and video

### Software defined radio


