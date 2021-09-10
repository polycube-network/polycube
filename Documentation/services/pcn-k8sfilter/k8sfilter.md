# K8sfilter


``k8sfilter`` is a small service that is attached to the physical interface of the nodes and performs a filtering on the incoming packets, if those packets are directed to ``NodePort`` services they are sent to the ``pcn-k8switch``, otherwise packets continue their journey to the Linux networking stack.

This service is not intended to be used alone but with [pcn-k8s](../../components/k8s/pcn-kubernetes).
