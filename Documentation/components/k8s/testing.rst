Testing pcn-k8s
===============

This document contains some commands to test that the deploying is working as expected.

In order to run these tests, a cluster having at least two schedulable nodes (not tainted) is needed.

Deploy services and pods to test
--------------------------------

.. parsed-literal::

    kubectl create -f |SCM_WEB|/src/components/k8s/examples/echoserver_nodeport.yaml
    kubectl run curl1 --image=tutum/curl --replicas=5 --command -- sleep 600000


After a short period of time, all pods should be in the `Running` state

::

    k8s@k8s-master:~$ kubectl get pods -o wide
    NAME                            READY     STATUS    RESTARTS   AGE       IP              NODE
    curl1-5df485555f-2dpwx          1/1       Running   0          1m        192.168.1.3     k8s-worker2
    curl1-5df485555f-l7ql4          1/1       Running   0          1m        192.168.0.246   k8s-master
    curl1-5df485555f-s9wsv          1/1       Running   0          1m        192.168.1.5     k8s-worker2
    curl1-5df485555f-xh4wv          1/1       Running   0          1m        192.168.1.4     k8s-worker2
    curl1-5df485555f-xqmxn          1/1       Running   0          1m        192.168.0.245   k8s-master
    myechoserver-86856fd86f-fkzg6   1/1       Running   0          32s       192.168.1.6     k8s-worker2


Tests
-----

The following section present some test cases to check everything is working as expected.

Test Node to Pod connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    # ping pod in master node
    k8s@k8s-master:~$ ping 192.168.0.245

    k8s@k8s-master:~$ ping 192.168.1.3


Test Pod to Pod connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    # select one pod running on master, in this case (192.168.0.245)
    k8s@k8s-master:~$ ID=curl1-5df485555f-xqmxn

    # ping pod in master
    k8s@k8s-master:~$ kubectl exec $ID ping 192.168.0.246

    # ping pod in worker
    k8s@k8s-master:~$ kubectl exec $ID ping 192.168.1.5


Test Pod to Internet connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    # ping to internet
    k8s@k8s-master:~$ kubectl exec $ID ping 8.8.8.8


Test ClusterIP service
^^^^^^^^^^^^^^^^^^^^^^

The following command will give us the details about the service we created:

::

    k8s@k8s-master:~$ kubectl describe service myechoserver
    Name:                     myechoserver
    Namespace:                default
    Labels:                   app=myechoserver
    Annotations:              <none>
    Selector:                 app=myechoserver
    Type:                     NodePort
    IP:                       10.96.23.23
    Port:                     <unset>  8080/TCP
    TargetPort:               8080/TCP
    NodePort:                 <unset>  31333/TCP
    Endpoints:                192.168.1.6:8080
    Session Affinity:         None
    External Traffic Policy:  Cluster
    Events:                   <none>

::

    # direct access to the backend
    k8s@k8s-master:~$ curl 192.168.1.6:8080

    # access from node to ClusterIP
    curl 10.96.23.23:8080

    # access from a pod (change ID to both, a pod in the local node and also a pod in a remote node)
    kubectl exec $ID curl 10.96.23.23:8080


Test NodePort service
^^^^^^^^^^^^^^^^^^^^^

The service is exposed in port `31333`, perform a request to the public IP of the master and the node from a remote host.

::

    # request to master
    curl 130.192.225.143:31333

    # request to worker
    curl 130.192.225.144:31333


TODO:
- test dns service
- test scale-up scale down
