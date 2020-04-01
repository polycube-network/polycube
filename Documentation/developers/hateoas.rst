Hateoas
=======

Polycube supports the Hateoas standard.
Thanks to this feature the daemon (:doc:`polycubed<../polycubed/polycubed>`) is able to provide the list
of valid endpoints to the client that can be used to make requests to the server.
The endpoints provided are those of the level just after the one requested.
From the :doc:`polycubed<../polycubed/polycubed>` root you can explore the service through the Hateoas standard.


What can Hateoas be used for?
-----------------------------

The Hateoas standard can be used to explore endpoints that can be used in polycube.
In this way a client will be able to access polycube resources and thus cubes independently.
This feature can also be useful in case you want to implement an alternative client to :doc:`polycubectl<../polycubectl/polycubectl>`.


How to use Hateoas
------------------
This feature can be used by observing the json of :doc:`polycubed<../polycubed/polycubed>` responses.
You can use an http client (e.g. Postman) to execute requests to the polycube daemon.



Example
-------

In this example if the client request is

::

  GET -> localhost:9000/polycube/v1/simplebridge/sb1/

then the server response will include all endpoints a of the level just after the simplebridge name (sb1).

::

  {
    "name": "sb1",
    "uuid": "d138da68-f6f4-4ee9-8238-34e9ab1df156",
    "service-name": "simplebridge",
    "type": "TC",
    "loglevel": "INFO",
    "ports": [
        {
            "name": "tovethe1",
            "uuid": "b2d6ebcb-163c-4ee8-a943-ba46b5fa4bda",
            "status": "UP",
            "peer": "veth1"
        },
        {
            "name": "tovethe2",
            "uuid": "e760a44d-5e9f-47e9-85d9-38e144400e83",
            "status": "UP",
            "peer": "veth2"
        }
    ],
    "shadow": false,
    "span": false,
    "fdb": {
        "aging-time": 300,
        "entry": [
            {
                "address": "da:55:44:5a:b1:b1",
                "port": "tovethe1",
                "age": 29
            },
            {
                "address": "11:ff:fe:80:00:00",
                "port": "tovethe2",
                "age": 26
            },
            {
                "address": "ee:c9:0e:19:c7:2a",
                "port": "tovethe2",
                "age": 1
            }
        ]
    },
    "_links": [
        {
            "rel": "self",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/"
        },
        {
            "rel": "uuid",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/uuid/"
        },
        {
            "rel": "type",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/type/"
        },
        {
            "rel": "service-name",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/service-name/"
        },
        {
            "rel": "loglevel",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/loglevel/"
        },
        {
            "rel": "ports",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/ports/"
        },
        {
            "rel": "shadow",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/shadow/"
        },
        {
            "rel": "span",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/span/"
        },
        {
            "rel": "fdb",
            "href": "//127.0.0.1:9000/polycube/v1/simplebridge/sb1/fdb/"
        }
    ]

}

As we can see from this answer json, the "_links" section contains all the endpoints
that the client can use to contact :doc:`polycubed<../polycubed/polycubed>` starting from the service name (sb1).

In this way a client can explore the service level by level.