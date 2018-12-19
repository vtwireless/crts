# The fast stream software design pattern

## Objective

Develop an API that provides the minimum number of interfaces needs to
make "streams" as we try to define here.  We wish to minimum number of
interfaces that we expose to the filter module (filter, component, block,
whatever => filter) writer so that filters may be reused.  By not letting
the filter control so much the filter becomes more reusable.  So we do not
let the filter have any control over how it runs, we let the question how
the filter runs be decided by a level above, like the end user, or a next
level middle-ware, but at all costs, not the filter.  The filter also does
not manage the connecting and passing of data between filters.  The filter
only sets limits on connecting and passing of data between filters.  This
restrictive filter definition distinguishes CRTS from all other "streams"
APIs (application programming interfaces).  

## Fast stream requirements

We are interested in a software design pattern that exposes it's structure
to the application layer.

Fast stream requires:

- filter modules.

- Filter modules read zero to some finite number of inputs from other
  modules and write zero to some finite number of outputs to other
  modules.

- The modules connect in a I/O (input/output) directed graph.

- The directed graph is not cyclic.

- We desire input/output flow from sources to sinks be fast.


Fast stream API trends:

- Buffering between modules is automatically managed making
  it somewhat seamless.

- Filter modules register filter parameters that may be
  monitored and adjusted with "control" interfaces.

- Controllers can monitored and adjusted filter modules parameters
  via the filter modules controls.


Running streams:

- The processes that run the filter modules should minimize the use
  of computer system resources in an automated/seamless fashion.





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


