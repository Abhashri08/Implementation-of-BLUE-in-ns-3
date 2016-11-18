# Implementation of BLUE algorithm in ns-3

## Course Code: CS822

## Assignment: #FP1

### Overview

Blue is an Active Queue Management (AQM) [1] mechanism like Random Early Detection (RED) [2], and has been implemented in ns-2 [3]. This repository contains an implementation of BLUE in ns-3 [4].

### BLUE Example

An example program for BLUE has been provided in

`src/traffic-control/examples/pfifo-vs-blue.cc`

and should be executed as

`./waf --run "pfifo-vs-blue --queueDiscType=BLUE"`

### References

[1] Feng, W. C., Kandlur, D., Saha, D., & Shin, K. (1999). BLUE: A new class of active queue management algorithms. Ann Arbor, 1001, 48105.

[2] Floyd, S., & Jacobson, V. (1993). Random early detection gateways for congestion avoidance. IEEE/ACM Transactions on networking, 1(4), 397-413.

[3] http://www.isi.edu/nsnam/ns/

[4] http://www.nsnam.org/
