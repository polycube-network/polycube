#!/bin/bash

kubectl delete --all deployments,services,networkpolicies,pods --namespace=pcn-test-default-ns
kubectl delete --all deployments,services,networkpolicies,pods --namespace=pcn-test-first-ns

kubectl delete namespace pcn-test-default-ns
kubectl delete namespace pcn-test-first-ns

