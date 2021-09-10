# Information for developers


## Controllers


Controllers are entities that are in charge of providing you with the resource that you need, as well as watch for events and notify when one has occurred.
In Polycube, five controllers are implemented:

- ``Kubernetes Network Policies`` 
- ``Polycube Network Policies``
- ``Services``
- ``Namespaces``
- ``Pods``

Not all of them provide the same functionalities and filtering criteria, but all work based on the same principle.


### Usage


The usage is inspired by Kubernetes' API style. 
To use the controllers, you simply need to import the ``pcn_controllers`` package and call the controller you'd like to use.
Take a look at the following examples.

```go
  
package main
import (
  // importing controllers
  pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
)

func main() {
      // Namespaces


      // Pods
      unsubscriptor, err := pcn_controllers.Namespaces().Subscribe(...)

     // etc...
}
```

### Queries


All controllers can retrieve resources from the Kubernetes cache, based on some criteria. 
To define criteria, you must define the query: the definition is in package ``pcn_types``:

```go
// ObjectQuery is a struct that specifies how the object should be found
type ObjectQuery struct {
By     string
Name   string
Labels map[string]string
}
```

Take a look at the following examples:

```go 
// I want resources that are named "my-service"
serviceQuery := pcn_types.ObjectQuery {
  By: "name",
  Name: "my-service",
}

// I want all pods that have labels "app: my-app", and "role: database"
podQuery := pcn_types.ObjectQuery {
  By: "labels",
  Labels: map[string]string {
    "app": "my-app",
    "role": "database",
}
```

Although you can create these by hand, a convenient function exists to do this and it is specifically made for use with the controllers:

```go
import (
  "github.com/polycube-network/polycube/src/components/k8s/utils"
)

// ...

// Build a "by: name" query
serviceQuery := utils.BuildQuery("my-service", nil)

// Build a "by: labels" query
podQuery := utils.BuildQuery("my-service", map[string]string {
      "app": "my-app",
      "role": "database",
})

// Build a query to get all resources, regardless of name and labels
allResources := utils.BuildQuery("", nil)
```

This function returns a **pointer** to the actual query structure because that's what controllers need. When wanting to get all resources, the function returns nil, so you may even just use a nil value without calling the BuildQuery function. 


### List resources


To list resources, you need to first create the queries, and then call the **List** function of the controller.
Not all controllers support both name and label criteria: i.e. the Pod Controller only supports labels.

```go

// I want all services that apply to pods with labels "app: my-app" and "role: db" 
// and are on a namespace called "production"
// So, first create the queries for both the service and namespace.
serviceQuery := utils.BuildQuery(nil, map[string]string {
      "app": "my-app",
      "role": "db",
})

nsQuery := utils.BuildQuery("production", nil)
// Then, get them. Note: there might be more than one service which applies to those pods.
servicesList, err := pcn_controllers.Services().List(serviceQuery, nsQuery)
if err != nil {
  return
}
for _, service := range servicesList {
  // Do something with this service...
}
```

So, usually, the first argument is criteria about the resource, while the second is reserved for criteria about the namespace where you want to find such resources.

To give additional information:

- The ``Kubernetes Network Policies`` and ``Polycube Network Policies`` controllers only support querying the policy by name
- The ``Pod`` controller only supports querying by labels
- The ``Pod`` controller also supports a third argument for the node where you want this pod to be located.
- The ``Services`` controller supports both name and labels, but when using labels it searches for them in the **spec.selector** field, not those under its metadata.
- The ``Namespaces`` controller work with namespaces, which cannot belong to other resources and only want one argument.

Note that, according to the criteria, it may take you a long time to get the results. Whenever possible, or when you expect a query to return lots of resources, adopt an async pattern or use multiple goroutines.


### Watch for events


To watch for events, you need to use a controller's ``Subscribe`` function by passing to it the event type you want to monitor, the resource criteria, and the function to be executed when that event is detected.

```go

func firstfunc() {
  // I want to "myfunc" to be notified whenever a new pod is born.
  // Pod controller has the most complex subscribe function, as it also asks you for the phase of the pod.
  unsub, err := pcn_controllers.Pods().Subscribe(pcn_types.New, nil, nil, &pcn_types.ObjectQuery{Name: "node-name"}, pcn_types.PodRunning, myfunc)

  // ... 

  // I am not interested in that event anymore
  unsub()
}
 
func myfunc(currentState, previousState *core_v1.Pod) {
  // Do something with it...
}
```

As the above example shows, the ``Subscribe`` function returns a pointer to a function that you need to call when you're not interested in that event anymore.

The function to execute must always have two arguments: the current state of the object and its previous state. There are three event types: ``New``, ``Update``, ``Delete``.

Just some heads up:

- When monitoring ``New`` events, only the current state of the object is present, the previous is obviously always ``nil``.
- When monitoring ``Delete`` events, the object does not exist anymore, so the current state is always ``nil``.

All the ``Subscribe`` functions share a similar structure to the ``List`` function in the same controller, to make sure about their usage, check their definitions in the ``pcn_controllers`` package


## Creating the Docker Images


Docker 18.06 is needed to build the images, and the daemon has to be started with the `--experimental` flag.
[See this issue to have more information](https://github.com/moby/moby/issues/32507).

```
export DOCKER_BUILDKIT=1 # flag needed to enable the --mount option
docker build --build-arg DEFAULT_MODE=pcn-k8s -t name:tag .
docker push name:tag
```