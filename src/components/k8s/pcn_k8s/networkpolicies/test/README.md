## Network Policy Tests
The test folder contains some tests to verify the behavior of the automatic security solution in Polycube.

## Prerequisites
All scripts need polycube to be running on all Kubernetes nodes.

## Architecture
Each test is composed of two Kubernetes namespaces: "pcn-test-default-ns" and "pcn-test-first-ns". In the "pcn-test-default-ns" there are two deployments that are used to check the policies: "target" and "no-target". 
The former is used to check if the policy is enforced correctly in case of Ingress policies, while the second is used to check if the solution selects the correct pods to protect. 
Finally, on both namespaces, there are some pods that will be used to contact the target deployment to check if traffic is really filtered or as pods to protect in case of Egress policies.
To test the solution's react component, some pods are deployed during the execution of the test, i.e. after the policy is deployed. 
All tests include a brief description of the current test case. 

## How to launch
In order to launch a test, run the test.sh found in the desired folder (Example: ingress/deny-all/test.sh).

## Tests:
Ingress Policies: All ingress policies are deployed in the "pcn-test-default-ns" namespace and applied to "target".
	- Deny All: A policy is deployed to reject all incoming connections. 
	- Limit Access:  A policy is deployed to only allow pods with label purpose=allowed on the same namespace as the target's to contact it.
	- Allow Namespace: This policy allows all pods from a specific namespace to contact the target. Any pod from a non-allowed namespace is blocked.
 	- Limit Namespace: This policy allows only pods with label purpose=allowed and on a specific namespace ("pcn-test-first-ns") to contact the target.
- Egress Policies: such policies are deployed to pods with label purpose=allowed on namespace "pcn-test-default-ns" and tested against the "target" deployment.
	- Limit Port: A policy is deployed to allow a pod to instantiate connections with the target only on port 8080. All other connections to the target should be blocked. Any connection to any other service must be blocked.

## Clean up
Each test will launch all the needed pods at start time if they are not there yet, but will *not* remove them: this is to save some time in case more tests are needed to be run later. The cleanup.sh script must be executed to fully remove the test's architecture.