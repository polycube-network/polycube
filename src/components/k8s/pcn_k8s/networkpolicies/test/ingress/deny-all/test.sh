#!/bin/bash

source ../../utils.sh

rand1=$RANDOM
rand2=$RANDOM

cleanDenyAll() {
	printf "\nCleaning up... "
	kubectl delete pod new-pod-$rand1 -n pcn-test-default-ns > /dev/null 2>&1
	kubectl delete pod new-pod-$rand2 -n pcn-test-first-ns > /dev/null 2>&1
    kubectl delete networkpolicy ingress-deny-all -n pcn-test-default-ns > /dev/null 2>&1

	printf "done.\n"
}
trap cleanDenyAll EXIT

denyAll() {
	echo "=================== DENY ALL ==================="
    echo "This test deploys a policy to the target pod that denies all incoming traffic."
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
	# Reject anything in ns
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should reject all connections from its namespace:\t"
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can acccess it by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can acccess it by cluster IP"
	    exit
	fi 

    printf "passed.\n"

	#-----------------------------------------
	# Reject anything on ns
	#-----------------------------------------

	printf "[PRE-EXISTING PODS] Target should reject all connections from other namespaces:\t"	
	testCurl "blocked" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-first-ns' can access it by target IP"
	    exit
	fi
    
    testCurl "blocked" "pcn-test-first-ns" "$targetClusterIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-first-ns' can access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

	#-----------------------------------------
	# Deploying new pods...
	#-----------------------------------------

	printf "Deploying new pods... "
	kubectl run new-pod-$rand1 --image=radial/busyboxplus:curl --restart=Never --labels purpose=testing,exists=new   -n pcn-test-default-ns 	--command -- sleep 3600 1>/dev/null
    kubectl run new-pod-$rand2 --image=radial/busyboxplus:curl --restart=Never --labels purpose=testing,exists=new   -n pcn-test-first-ns 		--command -- sleep 3600 1>/dev/null
	
	sameNs=""
	otherNs=""

	# Wait for them to be running
    while [ \( ${#sameNs} -eq 0 \) -o \( ${#otherNs} -eq 0 \) ]
    do
        sleep 2
		sameNs=$(kubectl get pods --selector exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
		otherNs=$(kubectl get pods --selector exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-first-ns)
	done

	printf "done.\n"
    sleep 5 # give time to the firewall

	#-----------------------------------------
	# Reject anything in ns (new)
	#-----------------------------------------

	printf "[NEW PODS] Target should reject all connections from new pods on its namespace:\t"
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
	# Reject anything in other ns (new)
	#-----------------------------------------

	printf "[NEW PODS] Target should reject all connections from new pods on other namespaces:\t"
    testCurl "new-pod-$rand2" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'new-pod-$rand2' on namespace 'pcn-test-first-ns' can access it by target IP"
	    exit
	fi 
	testCurl "new-pod-$rand2" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'new-pod-$rand2' on namespace 'pcn-test-first-ns' can access it by cluster IP"
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
    kubectl delete networkpolicy ingress-deny-all -n pcn-test-default-ns > /dev/null 2>&1
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

denyAll