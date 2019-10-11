#!/bin/bash

source ../../utils.sh

rand1=$RANDOM
rand2=$RANDOM

cleanAllowNamespace() {
	printf "\nCleaning up... "
	kubectl delete pods new-pod-$rand1 -n pcn-test-default-ns > /dev/null 2>&1
	kubectl delete pods new-pod-$rand2 -n pcn-test-first-ns > /dev/null 2>&1
    kubectl delete networkpolicy ingress-allow-namespace -n pcn-test-default-ns > /dev/null 2>&1
	printf "done.\n"
}
trap cleanAllowNamespace EXIT

allowNamespace() {
    echo "=================== ALLOW NAMESPACE ==================="
    echo "This test deploys a policy to the target pod that allows all connections from any pod from a specific namespace."
    echo ""

	# Get the IPs
    findTargetIPs
	
	# Wait for the pods to running
	sleep 10

	#-----------------------------------------
	# Curl without policy
	#-----------------------------------------

    printf "Target should allow connections from anyone:\t"
    testNoPolicy 
    if [ "$?" -gt "0" ]; then 
        exit
    fi
    printf "passed.\n"

	#-----------------------------------------
	# Deploy the policy
	#-----------------------------------------

    printf "Deploying the policy... "
    kubectl apply -f policy.yaml 1>/dev/null
	sleep 5
	printf "done.\n"

    #-----------------------------------------
	# Allow any pod ok namespace
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should allow all connections from an allowed namespace:\t"
    testCurl "allowed" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it by cluster IP"
	    exit
	fi
	testCurl "blocked" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-first-ns' cannot access it by target IP"
	    exit
	fi 
	testCurl "blocked" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-first-ns' cannot access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

	#-----------------------------------------
	# Reject all pods ko namespace
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should reject all connections from a blocked namespace:\t"
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access it by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access it by cluster IP"
	    exit
	fi
    testCurl "blocked" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' can access it by target IP"
	    exit
	fi 
	testCurl "blocked" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' can access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

    #-----------------------------------------
	# Deploying new pods...
	#-----------------------------------------

	printf "Deploying new pods... "
	kubectl run new-pod-$rand1 --image=radial/busyboxplus:curl --restart=Never --labels purpose=testing,exists=new   -n pcn-test-default-ns 	--command -- sleep 3600 1>/dev/null
    kubectl run new-pod-$rand2	--image=radial/busyboxplus:curl --restart=Never --labels purpose=testing,exists=new   -n pcn-test-first-ns 	    --command -- sleep 3600 1>/dev/null
    printf "done.\n"
	sleep 5
	#-----------------------------------------
	# Allow any pod ok namespace (new pods)
	#-----------------------------------------

    printf "[NEW PODS] Target should allow all connections from an allowed namespace:\t"
    allowedNs=""

	# Wait for them to be running
    while [ -z "$allowedNs" ]
    do
        sleep 2
		allowedNs=$(kubectl get pods --selector purpose=testing,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-first-ns)
    done

    testCurl "new-pod-$rand2" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'new-pod-$rand2' on namespace 'pcn-test-first-ns' cannot access it by target IP"
	    exit
	fi 
	testCurl "new-pod-$rand2" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'new-pod-$rand2' on namespace 'pcn-test-first-ns' cannot access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

	#-----------------------------------------
	# Reject all pods ko namespace (new pods)
	#-----------------------------------------

    printf "[NEW PODS] Target should reject all connections from a blocked namespace:\t"

    blockedNs=""
    while [ ${#blockedNs} -eq 0 ]
    do
        sleep 2
		blockedNs=$(kubectl get pods --selector purpose=testing,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
    done

    testCurl "new-pod-$rand1" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'new-pod-$rand1' on namespace 'pcn-test-default-ns' can access it by target IP"
	    exit
	fi 
	testCurl "new-pod-$rand1" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'new-pod-$rand1' on namespace 'pcn-test-default-ns' can access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

    #-----------------------------------------
	# Test pods that do not match the policy
	#-----------------------------------------

    printf "Pods that do not match this policy are still accessible:\t"
    testNoTarget
    if [ "$?" -gt "0" ]; then 
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Test without policy again
	#-----------------------------------------

    printf "Removing the policy... "
    kubectl delete networkpolicy ingress-allow-namespace -n pcn-test-default-ns > /dev/null 2>&1
	sleep 5
	printf "done.\n"

	printf "Target should allow all connections again:\t"
    testNoPolicy 
    if [ "$?" -gt "0" ]; then 
        exit
    fi
    printf "passed.\n"

    printf "All tests passed.\n"
}
allowNamespace