# Simple Forwarder


Simple forwarder is another sample service, in this case it is more advanced than the HelloWold service, it is used both, for internal testing and to show the usage of more complex features.

## Description


Simple forwarder contains a primitive forwarding table that according to the packet's ingress port it can either:
- send the packet to another port
- send the packet to the slow path
- drop the packet

## How to use


TODO: api is document in that way, how to do it?

The api for the forwarder service can be found [here](https://app.swaggerhub.com/apis/netgrp-polito/forwarder-api/1.0.0).
The forwarding table of the service can be modified through the `.../actions/` api, it allows to add an action for each port.
