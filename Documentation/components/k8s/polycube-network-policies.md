# Polycube Policies


pcn-k8s CNI implementes both [standard kubernetes networking policy](kubernetes-network-policies) and advanced Polycube networking policies.
The latter provide a more flexible, simpler and more advanced approach to filter the traffic that a pod should allow or block.
They include all the features from Kubernetes Network Policies and present some additional features, which are going to be described here.

The isolation mode, a core concept in Kubernetes Network Policies, is followed by Polycube Network Policies, as well: pods allow all communication until a policy - either one - is applied to them, and at that moment they will start to allow only what's explicitly specified in the policy.

## Human Readable Policies


One of the goals of the Polycube Policies is to also encourage operators to write more effective policies, and the very first thing that can help this process is a friendly and understandable syntax.

In Polycube Policies, there is no need to use the ``[]`` and ``{}`` operators in multiple parts of the policy to select no/all pods: two convenient fields - ``dropAll`` and ``allowAll`` - are there just for this purpose, leaving no room for confusion.

```yaml
apiVersion: polycube.network/v1beta
kind: PolycubeNetworkPolicy
metadata: 
  name: deny-all
applyTo:
  target: pod
  withLabels: 
    role: db
spec:
  ingressRules:
    dropAll: true 
```

As shown, Polycube policies follow the natural language that humans speak - humans with a basic understanding of the English language, that is. As a matter of fact, when selecting the pods that the policy must be applied to, the ``applyTo`` field must be used, and the ``target`` field clearly specifies the need to protect a ``pod`` that has labels ``role: db``, as finally stated in the ``withLabels`` field.

If you want to apply a policy to all pods on a given namespace, the following syntax may be used:

```yaml
applyTo:
  target: pod
  any: true
```

## Automatic type detection


There is no need to specify the policy type - ``Ingress`` or ``Egress`` - as it is automatically recognized, based on their existence in the policy YAML file.

## Prevent Direct Access


if you want your pods to be contacted only through their Service(s) and block direct access, set the ``preventDirectAccess`` to ``true``.

```yaml
apiVersion: polycube.network/v1beta
kind: PolycubeNetworkPolicy
metadata: 
  name: pod-drop-all
priority: 3
ingressRules:
  preventDirectAccess: true
  rules: ...
```

From now on, the pods will only allow connections that were made through their ``ClusterIP``.

## Explicit Priority

Priority can explicitly be declared by properly writing it in the policy, where a lower number indicates its importance in rules insertion.

It can be done by setting the ``priority`` field:

```yaml
apiVersion: polycube.network/v1beta
kind: PolycubeNetworkPolicy
metadata: 
  name: pod-drop-all
priority: 3
```

Since more recent policies always take priority (and thus are checked first), setting and explicit priority can help in those situations where you want to a policy to be checked before others. 

Just to make an example: if you'd like to temporarily block all traffic to check for anomalies, there is no need for you to remove all existing policies and deploy one that drops all traffic, as you can simply give the latter a higher priority (i.e. 1) and deploy it: the higher priority will make it the first one to be checked and, as a result, all traffic would be blocked without modifying the other policies.

## Strong distinction between the internal and external


The rules that can be specified are divided by what is internal to the cluster and what is outside. 

This is done to prevent the clear bad behavior of using ``IPBlock`` to target pods. Peers are divided in two groups: ``pod`` and ``world``.

The internal world can be specified like so:

```yaml
ingressRules:
  rules:
    - action: allow
      from:
        peer: pod
        withLabels:
          role: api
        onNamespace:
          withNames:
            - beta
            - production
```

Once again, the syntax closely resembles a natural spoken language.

In Kubernetes Network Policies, namespaces can be targeted only by the labels they have: when wanting to target them, the operator is forced to assign labels to namespaces even if they just need to target very few of them. As the policy above shows, Polycube Policies provide a way to select namespaces by their names as well, while also providing the ability to do so by their labels.

The external world, instead, can be restricted by writing ``world`` in the ``peer`` field.

```yaml
ingressRules:
  rules:
    - action: drop
      from: 
        peer: world
        withIP:
          -  100.100.100.0/28
    - action: allow
      from: 
        peer: world
        withIP:
          -  100.100.100.0/24
```

So, there is no need to write ``exceptions``, as in Kubernetes Network Policies, because Polycube policies also have a clear distinction between actions.

## Drop or Allow actions


```yaml
ingressRules:
  rules:
  - from:
      peer: pod
        withLabels:
          role: api
      action: drop
```

To allow a connection, the actions that can be written are ``allow``, ``pass``, ``forward`` and ``permit``.

The same applies when blocking connections, and the following words can be used: ``block``, ``drop``, ``prohibit`` and ``forbid``.

The presence of multiple words to define a single action has been done to aid the definition of a policy, allowing for a more flexible semantic that is easier to remember.

This will help you create ``Blacklist``-style policies by creating two or more policies: one, with a lower priority, that allows all pods in a certain port/protocol, and another one (or more) that will work as a blacklist of pods banned (i.e. those that are in the ``beta`` namespace).

This was a clear example of the flexibility of the Polycube Policies, but one must take very careful steps when creating a blacklist policy: although this could introduce some benefits, like lighter firewalls, it could also add some subtle inconsistencies and errors if are not created mindfully, like wrongly allowing pods to start connections.

## Service-aware policies


Consider the following Polycube policy:

```yaml
apiVersion: polycube.network/v1beta
kind: PolycubeNetworkPolicy
metadata: 
  name: service-allow-api
applyTo:
  target: service
  withName: database
spec:
  ingressRules:
    rules:
      - from:
          peer: pod
          withLabels:
            role: api
        action: allow
```

By writing ``service`` as a ``target``, Polycube will be aware of the fact that pods have a service applied to them and will make all the necessary steps to protect the pods according to it.

Supposing that service named ``database`` has ``80`` and ``443`` as ``targetPorts`` with protocol ``TCP``, all the pods that apply such service will accept connections from pods that have label ``role: api``, but only on the aforementioned ports and protocol. 

This serves both as a convenient method for targeting pods without specifying the labels - ``withName: database`` can be seen as a clear shortcut in this case - and without specifying the ports as well. 

Being ``service-aware`` means that firewalls will react to ``Service`` events, too: if later, the cluster's needs change and only the more secure ``443`` port is decided to be supported, the service can be updated to reflect this change and the solution will react as well by removing the behavior it used to apply for port ``8080``.

The service-aware functionality is made for those particular use cases when a pod does not need a more advanced rule filtering, like allowing a pod on a certain port and allowing others on another one: as already mentioned, this is a convenient method for specifying all ports at once, and if such scenario is needed, it must be done by specifying ``pod`` as the peer instead of using ``service``.

As a final note, only services with selectors are supported: services without selectors need to be selected by writing ``world`` as the peer.