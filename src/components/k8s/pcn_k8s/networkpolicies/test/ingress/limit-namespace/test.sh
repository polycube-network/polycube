#!/bin/bash

source ../../utils.sh

rand1=$RANDOM
rand2=$RANDOM
rand3=$RANDOM
rand4=$RANDOM

cleanLimitNamespace() {
	printf "\nCleaning up... "
	kubectl delete pods allowed-new-$rand2 -n pcn-test-default-ns > /dev/null 2>&1
	kubectl delete pods blocked-new-$rand3 -n pcn-test-default-ns > /dev/null 2>&1
	kubectl delete pods allowed-new-$rand1 -n pcn-test-first-ns > /dev/null 2>&1
	kubectl delete pods blocked-new-$rand4 -n pcn-test-first-ns > /dev/null 2>&1
    kubectl delete networkpolicy ingress-limit-namespace -n pcn-test-default-ns > /dev/null 2>&1
	printf "done.\n"
}
trap cleanLimitNamespace EXIT

limitNamespace() {
	echo "=================== LIMIT NAMESPACE ==================="
    echo "This test deploys a policy to the target pod that only allows pods with specific labels and on a specific namespace to contact it."
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
	# Allow ok labels ok namespace
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should allow connections from pods with proper labels and on allowed namespaces:\t"
    testCurl "allowed" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' can access it by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' can access it by cluster IP"
	    exit
	fi
    printf "passed.\n"

    #-----------------------------------------
	# Allow ok labels ko namespace (pre-existing) 
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should reject connections from pods that have the proper labels but are on the wrong namespace:\t"
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

    printf "passed.\n"

	#-----------------------------------------
	# Rejects ko labels any ns
	#-----------------------------------------

    printf "[PRE-EXISTING PODS] Target should reject connections from pods that don't have the proper labels, regardless of the namespace:\t"
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
	kubectl run allowed-new-$rand1 --image=radial/busyboxplus:curl --restart=Never --labels purpose=allowed,exists=new   -n pcn-test-first-ns 		--command -- sleep 3600 1>/dev/null
	kubectl run allowed-new-$rand2 --image=radial/busyboxplus:curl --restart=Never --labels purpose=allowed,exists=new   -n pcn-test-default-ns 	--command -- sleep 3600 1>/dev/null

	kubectl run blocked-new-$rand3 --image=radial/busyboxplus:curl --restart=Never --labels purpose=blocked,exists=new   -n pcn-test-default-ns 	--command -- sleep 3600 1>/dev/null
	kubectl run blocked-new-$rand4 --image=radial/busyboxplus:curl --restart=Never --labels purpose=blocked,exists=new   -n pcn-test-first-ns 		--command -- sleep 3600 1>/dev/null

	# Wait for the first one to be running
	firstNsAllowed=""

    while [ ${#firstNsAllowed} -eq 0  ]
    do
        sleep 2
		firstNsAllowed=$(kubectl get pods --selector purpose=allowed,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-first-ns)
    done

	printf "done.\n"
	sleep 5
	#-----------------------------------------
	# Allow ok labels ok ns (new pods)
	#-----------------------------------------

    printf "[NEW PODS] Target should allow connections from new pods on allowed namespaces:\t"
    testCurl "allowed-new-$rand1" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed-new-$rand1' on namespace 'pcn-test-first-ns' cannot access it by target IP"
	    exit
	fi 
	testCurl "allowed-new-$rand1" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed-new-$rand1' on namespace 'pcn-test-first-ns' cannot access it by cluster IP"
	    exit
	fi

    printf "passed.\n"

	#-----------------------------------------
	# Reject ok labels ko namespace (new pods)
	#-----------------------------------------

	defaultNsAllowed=""

	# Wait for it to be running
    while [ ${#defaultNsAllowed} -eq 0  ]
    do
        sleep 2
		defaultNsAllowed=$(kubectl get pods --selector purpose=allowed,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
    done

    printf "[NEW PODS] Target should reject connections from new pods that have the proper labels but are on the wrong namespace:\t"
    testCurl "allowed-new-$rand2" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed-new-$rand2' on namespace 'pcn-test-default-ns' can access it by target IP"
	    exit
	fi 
	testCurl "allowed-new-$rand2" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'allowed-new-$rand2' on namespace 'pcn-test-default-ns' can access it by cluster IP"
	    exit
	fi

    printf "passed.\n"

	#-----------------------------------------
	# Reject ko labels any ns
	#-----------------------------------------

	defaultWrong=""
	firstWrong=""

	# Wait for them to be running
    while [ \( ${#defaultWrong} -eq 0 \) -o \( ${#firstWrong} -eq 0 \)   ]
    do
        sleep 2
		defaultWrong=$(kubectl get pods --selector purpose=blocked,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
		firstWrong=$(kubectl get pods --selector purpose=blocked,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-first-ns)
    done

	printf "[NEW PODS] Target should reject connections from new pods that don't have the proper labels, regardless of the namespace:\t"
    testCurl "blocked-new-$rand4" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked-new-$rand4' on namespace 'pcn-test-first-ns' can access it by target IP"
	    exit
	fi 
	testCurl "blocked-new-$rand4" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked-new-$rand4' on namespace 'pcn-test-first-ns' can access it by cluster IP"
	    exit
	fi
    testCurl "blocked-new-$rand3" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked-new-$rand3' on namespace 'pcn-test-default-ns' can access it by target IP"
	    exit
	fi 
	testCurl "blocked-new-$rand3" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -eq "0" ]; then
	    printf "failed. Pod 'blocked-new-$rand3' on namespace 'pcn-test-default-ns' can access it by cluster IP"
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
    kubectl delete networkpolicy ingress-limit-namespace -n pcn-test-default-ns > /dev/null 2>&1
	sleep 5
	printf "done.\n"

	printf "Target should allow all connections again:\t"
    testNoPolicy 
    if [ "$?" -gt "0" ]; then 
        exit
    fi
    printf "passed.\n"

    echo "All tests passed.\n"
}
limitNamespace