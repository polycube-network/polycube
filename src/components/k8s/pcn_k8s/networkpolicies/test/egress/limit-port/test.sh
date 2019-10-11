#!/bin/bash

source ../../utils.sh

rand1=$RANDOM

cleanLimitPort() {
	printf "\nCleaning up... "
	kubectl delete pods new-pod-$rand1 -n pcn-test-default-ns > /dev/null 2>&1
    kubectl delete networkpolicy egress-limit-namespace -n pcn-test-default-ns > /dev/null 2>&1
	printf "done.\n"
}
trap cleanLimitPort EXIT

limitPort() {
	echo "=================== LIMIT PORTS ==================="
    echo "This test deploys a policy to a pod and allows it to contact a pod only through a certain por and protocol."
    echo ""

    # Get the IPs
    findTargetIPs
	
	# Wait for the pods to running
	sleep 10

	#-----------------------------------------
	# Curl without policy
	#-----------------------------------------

    printf "[PRE-EXISTING POD] Pod should be able to contact target on any available port:\t"
    testEgressNoPolicy
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
	# Allow pod on port 8080
	#-----------------------------------------

    printf "[PRE-EXISTING POD] Pod should bel allowed to contact target on 8080:\t"
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Forbid pod on port 80
	#-----------------------------------------

    printf "[PRE-EXISTING POD] Pod should be forbidden from contacting target on 80:\t"
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 80 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 80 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Forbid pod on any other service
	#-----------------------------------------

    printf "[PRE-EXISTING POD] Pod should be forbidden from contacting any other service, regardless of the port:\t"
    testCurl "allowed" "pcn-test-default-ns" "$noTargetIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 80 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetClusterIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 80 through its cluster IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetIP" "8080"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 8080 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetClusterIP" "8080"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 8080 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Deploying new pods...
	#-----------------------------------------

	printf "Deploying a new pod... "
	kubectl run new-pod-$rand1 --image=radial/busyboxplus:curl --restart=Never --labels purpose=allowed,exists=new   -n pcn-test-default-ns --command -- sleep 3600 1>/dev/null
	
	# Wait for the first one to be running
	firstNsAllowed=""

    while [ ${#firstNsAllowed} -eq 0  ]
    do
        sleep 2
		firstNsAllowed=$(kubectl get pods --selector purpose=allowed,exists=new -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
    done

	printf "done.\n"
    sleep 5

    #-----------------------------------------
	# Allow pod on port 8080
	#-----------------------------------------

    printf "[NEW POD] Pod should be allowed to contact target on 8080:\t"
    testCurl "new-pod-$rand1" "pcn-test-default-ns" "$targetIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'new-pod-$rand1' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its target IP."
        exit
    fi
    testCurl "new-pod-$rand1" "pcn-test-default-ns" "$targetClusterIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'new-pod-$rand1' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Forbid pod on port 80
	#-----------------------------------------

    printf "[NEW POD] Pod should be forbidden from contacting target on 80:\t"
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 80 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 80 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# Forbid pod on any other service
	#-----------------------------------------

    printf "[NEW POD] Pod should be forbidden from contacting any other service, regardless of the port:\t"
    testCurl "allowed" "pcn-test-default-ns" "$noTargetIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 80 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetClusterIP" "80"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 80 through its cluster IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetIP" "8080"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' can access on port 8080 through its target IP."
        exit
    fi
    testCurl "allowed" "pcn-test-default-ns" "$noTargetClusterIP" "8080"
    if [ "$?" -eq "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cann access on port 8080 through its cluster IP."
        exit
    fi
    printf "passed.\n"

    #-----------------------------------------
	# No policy match
	#-----------------------------------------

    printf "Pods that do not match this policy are allowed to contact the target on any port:\t"

    # Same namespace
    testCurl "blocked" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' cannot access it on 80 by target IP"
	    exit
	fi 
	testCurl "blocked" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' cannot access it on 80 by cluster IP"
	    exit
	fi
    testCurl "blocked" "pcn-test-default-ns" "$targetIP" "8080"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' cannot access it on 8080 by target IP"
	    exit
	fi 
	testCurl "blocked" "pcn-test-default-ns" "$targetClusterIP" "8080" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'blocked' on namespace 'pcn-test-default-ns' cannot access it on 8080 by cluster IP"
	    exit
	fi

    # Another namespace
    testCurl "allowed" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it on 80 by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it on 80 by cluster IP"
	    exit
	fi
    testCurl "allowed" "pcn-test-first-ns" "$targetIP" "8080"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it on 8080 by target IP"
	    exit
	fi 
	testCurl "allowed" "pcn-test-first-ns" "$targetClusterIP" "8080" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it on 8080 by cluster IP"
	    exit
	fi
    printf "passed.\n"

	#-----------------------------------------
	# Test without policy again
	#-----------------------------------------

    printf "Removing the policy... "
    kubectl delete networkpolicy egress-limit-port -n pcn-test-default-ns > /dev/null 2>&1
	sleep 5
	printf "done.\n"

    printf "Pod should be allowed to contact target on any available port again:\t"
    testEgressNoPolicy
    if [ "$?" -gt "0" ]; then
        exit
    fi
    printf "passed.\n"
    echo "All tests passed.\n"
}
limitPort