#!/bin/bash

# The IPs
targetIP=""
noTargetIP=""
targetClusterIP=""
noTargetClusterIP=""

testCurl() {
    result=$(kubectl exec $1 -n $2 -i -t -- curl -m 5 $3:$4 2>/dev/null)
    if [[ $result == hello* ]]; then 
        return 0
    else 
        return 1
    fi
}

testNoPolicy(){
	# Same Namespace
	testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access it by target IP"
	    return 1
	fi 
	testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access it by cluster IP"
	    return 1
	fi 

	# Different Namespace
	testCurl "allowed" "pcn-test-first-ns" "$targetIP" "80"
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it by target IP"
	    return  1
	fi 
	testCurl "allowed" "pcn-test-first-ns" "$targetClusterIP" "80" 
	if [ "$?" -gt "0" ]; then
	    printf "failed. Pod 'allowed' on namespace 'pcn-test-first-ns' cannot access it by cluster IP"
	    return 1
	fi 

    return 0
}

testEgressNoPolicy(){
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "80"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 80 through its target IP."
        return 1
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "80"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 80 through its cluster IP."
        return 1
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its target IP."
        return 1
    fi
    testCurl "allowed" "pcn-test-default-ns" "$targetClusterIP" "8080"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' on namespace 'pcn-test-default-ns' cannot access on port 8080 through its cluster IP."
        return 1
    fi

    return 0
}

testNoTarget() {
    # Same namespace
    testCurl "allowed" "pcn-test-default-ns" "$noTargetIP" "80"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' cannot access it by target IP"
        return 1
    fi 
    testCurl "allowed" "pcn-test-default-ns" "$noTargetClusterIP" "80" 
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' cannot access it by cluster IP"
        return 1
    fi 

    # Other namespace
	testCurl "allowed" "pcn-test-first-ns" "$noTargetIP" "80"
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' cannot access it by target IP"
        return 1
    fi 
    testCurl "allowed" "pcn-test-first-ns" "$noTargetClusterIP" "80" 
    if [ "$?" -gt "0" ]; then
        printf "failed. Pod 'allowed' cannot access it by cluster IP"
        return 1
    fi
}

createNs() {
    # Create the namespace
    kubectl create namespace pcn-test-default-ns    1>/dev/null
    kubectl create namespace pcn-test-first-ns      1>/dev/null

    # Label them
    kubectl label ns/pcn-test-default-ns role=default   1>/dev/null
    kubectl label ns/pcn-test-first-ns   role=first     1>/dev/null
}

createTargets() {
    kubectl create deployment target    --image=asimpleidea/simple-service:latest -n pcn-test-default-ns    1>/dev/null
    kubectl create deployment no-target --image=asimpleidea/simple-service:latest -n pcn-test-default-ns    1>/dev/null

    kubectl create service clusterip target     -n pcn-test-default-ns --tcp 80:80 --tcp 8080:8080 1>/dev/null
    kubectl create service clusterip no-target  -n pcn-test-default-ns --tcp 80:80 --tcp 8080:8080 1>/dev/null
}

createPods() {
    # Create the jobs
    kubectl run allowed --image=radial/busyboxplus:curl --restart=Never --labels purpose=allowed    -n pcn-test-default-ns --command -- sleep 36000 1>/dev/null
    kubectl run blocked --image=radial/busyboxplus:curl --restart=Never --labels purpose=blocked    -n pcn-test-default-ns --command -- sleep 36000 1>/dev/null

    kubectl run allowed --image=radial/busyboxplus:curl --restart=Never --labels purpose=allowed    -n pcn-test-first-ns --command -- sleep 36000 1>/dev/null
    kubectl run blocked --image=radial/busyboxplus:curl --restart=Never --labels purpose=blocked    -n pcn-test-first-ns --command -- sleep 36000 1>/dev/null
}

findTargetIPs() {

    echo "Getting target IP..."
    kubectl get deployments target --template={{.metadata.name}} -n pcn-test-default-ns > /dev/null 2>&1
    if [ $? -gt 0 ]; then
        echo "Target pod not found. Deploying it now..." 
        createNs
        createTargets
        createPods
    fi

    while [ \( ${#targetIP} -eq 0 \) -o \( ${#noTargetIP} -eq 0 \) ]
    do
        sleep 2

        targetIP=$(kubectl get pods --selector app=target -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
        targetClusterIP=$(kubectl get -o template service/target --template={{.spec.clusterIP}} -n pcn-test-default-ns)

        noTargetIP=$(kubectl get pods --selector app=no-target -o=jsonpath='{.items[0].status.podIP}' -n pcn-test-default-ns)
        noTargetClusterIP=$(kubectl get -o template service/no-target --template={{.spec.clusterIP}} -n pcn-test-default-ns)
    done

    echo "Done."
    sleep 5
}
