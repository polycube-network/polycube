pcn-k8s Developers
==================

Creating the Docker Images
--------------------------

Docker 18.06 is needed to build the images, and the daemon has to be started with the `--experimental` flag.
`See this issue to have more information <https://github.com/moby/moby/issues/32507>`_.

::
    export DOCKER_BUILDKIT=1 # flag needed to enable the --mount option
    docker build --build-arg MODE=pcn-k8s -t name:tag .
    docker push name:tag
