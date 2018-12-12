pcn-k8s Developers
==================

Creating the Docker Images
--------------------------

Docker 18.06 is needed to build the images, and the daemon has to be started with the `--experimental` flag.
`See this issue to have more information <https://github.com/moby/moby/issues/32507>`_.

You also need to ``export DOCKER_BUILDKIT=1`` before building the image

::

    docker build --build-arg KUBERNETES=true -t polycubenetwork/k8s-pod-network -f Dockerfile .
    docker push polycubenetwork/k8s-pod-network


Creating image for internal developing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In order to speed up the creation of the images for development, there are two different images

The base image contains all the dependencies required by `polycubed` and by the agents, usually this image only has to be created the first time.
The following command creates the base image and tags it as `pcn_base`

::

    cd polycube
    docker build --build-arg KUBERNETES=true -t polycube_base -f Dockerfile.base .


The second image uses the first one as base and adds ``polycubed`` and the different agents.
Two labels are used for the image

 - `dev` used in the developing process
 - `stable` contains a tested and working solution

The following commands create, tag and push the image to the docker registry.

::

    docker build --build-arg KUBERNETES=true -t polycubenetwork/k8s-pod-network -f Dockerfile.polycube .
    docker push polycubenetwork/k8s-pod-network
