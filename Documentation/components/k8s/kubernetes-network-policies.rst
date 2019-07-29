Kubernetes Network Policies
==============================================

``pcn-k8s`` leverages on the Kubernetes Network Policies to protect the pods of your cluster from both external and internal unauthorized traffic, as well as preventing them from accessing other pods without permission.

All pods will be able to communicate with anyone as long as no policy is applied to them: this is called *Non-Isolation Mode*. As soon as one is deployed and applied to them, all traffic is going to be dropped, except for what is specified inside the policy, according to the direction it is filtering: incoming (``ingress``) or outgoing (``egress``) traffic.

Only ``IP`` traffic is restricted and both ``TCP`` and ``UDP`` are supported as the transport protocol.
Kubernetes Network Policies are written in a ``yaml`` format and always include the following fields:

.. code-block:: yaml

    kind: NetworkPolicy
    apiVersion: networking.k8s.io/v1
    metadata:
      name: policy-name
      namespace: namespace-name

If the ``namespace`` field is omitted, the policy will be deployed to the ``default`` namespace.

Select the pods
------------------

In order to select the pods to be protected, the ``podSelector`` field, under ``spec`` must be filled.

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: policy-name
  spec:
    podSelector:
      matchLabels:
        app: my-app

If a pod has a label named ``app`` and its value is equal to ``my-app``, the above will be applied to it, regardless of its other labels.

``podSelector`` only supports ``matchLabels``, as ``matchExpression`` is not supported yet.

To select all pods the following syntax can be used:

.. code-block:: yaml
  
  spec:
    podSelector: {}

Restrict the traffic
-----------------------
Both incoming and outgoing traffic can be restricted. In order to restrict incoming packets, the ``ingress`` field must be filled:

.. code-block:: yaml
  
  spec:
    ingress:
    - from:
      # Rule 1...
    - from:
      # Rule 2...
    - from:
      # Rule 3...

Outgoing traffic follows a very similar pattern:

.. code-block:: yaml
  
  spec:
    egress:
    - to:
      # Rule 1...
    - to:
      # Rule 2...
    - to:
      # Rule 3...

**NOTE**: in case of restricting outgoing traffic, ``policyTypes`` - under ``spec`` - must be filled like so:

.. code-block:: yaml
  
  policyTypes:
  - Ingress # Only if also restricting incoming traffic
  - Egress

``policyTypes`` can be ignored if the policy is ``ingress``-only.

Allow external hosts
-----------------------------
The field ``ipBlock`` must be filled with the IPs of the hosts to allow connections from/to, written in a ``CIDR`` notation. Exceptions can be optionally specified.

.. code-block:: yaml
  
  ingress:
  - from:
    - ipBlock:
        cidr: 172.17.0.0/16
        except:
        - 172.17.1.0/24

The ips of the pods inside of the cluster are not fixed: as a result, ``ipBlock`` must not be used to restrict access from other pods in the cluster, but only for external entities.

Allow Pods
-----------------------------
Access from other pods is restricted by using the ``podSelector`` - introduced earlier - and ``namespaceSelector`` fields. The latter works in the same fashion as the former, selecting namespaces by their label.

- In case of only using ``podSelector`` only the pods following the criteria specified by it and that are in the same namespace as the one specified under ``metadata`` will be able to access the pod.

- If only ``namespaceSelector`` is used, all pods contained inside the namespace with the labels specified in it will be granted access to the pod.

- For a more fine-grained selection, both can be used to specifically select pods with certain labels and that are on namespaces with specific labels.

Look at the examples in the example section to learn more about their usage.

Ports and protocols
-----------------------------

In order to define protocols, one must use the ``ports`` field, under the ``from``/``to`` depending on the direction filtering:

.. code-block:: yaml
  
  ingress:
  - from:
    # rule...     
    ports:
    - port: 5000
      protocol: TCP
 
Refer to the examples section for more details.

Deploy and Remove
-----------------------------

In order to deploy the policy, the typical ``apply`` command must be entered:

::
 
  # Local policy
  kubectl apply -f path-to-policy.yaml

  # Remote policy
  kubectl apply -f https://example.com/policy.yaml


To remove, one of the following commands can be issued:

::

  # Delete by its path
  kubectl delete -f path-to-policy.yaml

  # Delete by its name
  kubectl delete networkpolicy policy-name


Examples
-----------------------------

Deny all traffic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following policy will be applied to pods that are on namespace ``production`` and have label ``app: bookstore``. It drops all incoming traffic.

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: api-allow
    namespace: production
  spec:
    podSelector:
      matchLabels:
        app: bookstore
  ingress: []

The ``[]`` selects no pods: it drops all traffic from the specified direction.

Accept all traffic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following policy will be applied to pods that are on namespace ``production`` and have label ``app: bookstore``. It accepts all incoming traffic.

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: api-allow
    namespace: production
  spec:
    podSelector:
      matchLabels:
        app: bookstore
   ingress:
   - {}

The ``{}`` selects everything regardless of labels: it accepts all traffic in the specified direction.


Limit connections to pods on the same namespace
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following policy will be applied to pods that are on namespace ``production`` and have label ``app: bookstore``. It accepts connections from all pods that have labels ``app: bookstore`` and ``role: api`` and that are on the **same namespace** as the policy's one.

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: api-allow
    namespace: production
  spec:
    podSelector:
      matchLabels:
        app: bookstore
  ingress:
  - from:
      - podSelector:
          matchLabels:
            role: api
            app: bookstore

Allow connections from all pods on a namespace
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following policy will be applied to pods that are on namespace ``production`` and have label ``app: bookstore``. It accepts connections from **all** pods that are running on namespaces that include the label ``app: bookstore``. 

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: api-allow
    namespace: production
  spec:
    podSelector:
      matchLabels:
        app: bookstore
  ingress:
  - from:
      - namespaceSelector:
          matchLabels:
            app: bookstore

Allow connections only from specific pods on specific namespaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following policy will be applied to pods that are on namespace ``production`` and have label ``app: bookstore``. It accepts connections from pods that have label ``role: api``, running on namespaces that include the label ``app: bookstore``. 

.. code-block:: yaml
  
  kind: NetworkPolicy
  apiVersion: networking.k8s.io/v1
  metadata:
    name: api-allow
    namespace: production
  spec:
    podSelector:
      matchLabels:
        app: bookstore
  ingress:
  - from:
      - podSelector:
          matchLabels:
            role: api
        namespaceSelector:
          matchLabels:
            app: bookstore

Rules and protocols combination
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Rules are OR-ed with each other and are AND-ed with the protocols:

.. code-block:: yaml
  
  ingress:
  - from:
    # Rule 1
    - ipBlock:
        cidr: 172.17.0.0/16
        except:
        - 172.17.1.0/24
    # Rule 2
    - podSelector:
        matchLabels:
          role: frontend
    ports:
    # Protocol 1
    - protocol: TCP
      port: 80
    # Protocol 2
    - protocol: TCP
      port: 8080

In the example above, a packet will be forwarded only if it matches one of the following conditions:

- ``Rule 1`` **AND** ``Protocol 1``
- ``Rule 1`` **AND** ``Protocol 2``
- ``Rule 2`` **AND** ``Protocol 1``
- ``Rule 2`` **AND** ``Protocol 2``

If none of the above applies, the packet is dropped.

Outgoing traffic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All the above policies can also apply to outgoing traffic by replacing ``ingress`` with ``egress`` and ``from`` with ``to``.
Also, ``policyTypes`` must be filled accordingly.

*NOTE*: keep in mind that as soon as the policy is deployed, all unauthorized traffic will be dropped, and this includes DNS queries as well! So, a rule allowing DNS queries on port 52 can be specified to prevent this.

Resources
-----------------------
For additional information about the Kubernetes Network Policies, please refer to the `official documentation <https://kubernetes.io/docs/concepts/services-networking/network-policies/>`_.