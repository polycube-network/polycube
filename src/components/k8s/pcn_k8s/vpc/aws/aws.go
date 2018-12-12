/*
 * Copyright 2018 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package awsvpc

import (
	"errors"
	"fmt"
	"net"

	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/kv"
	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/vpc"

	"github.com/vishvananda/netlink"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/ec2metadata"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/ec2"

	log "github.com/sirupsen/logrus"
)

const (
	AmsRouteTableId string = "routeTableId"
)

type AwsVpc struct {
	routeTableId string
	eniId        string
	ec2Svc       *ec2.EC2
	route        []*net.IPNet
	kv           kv.KV
	kvPrefix     string
}

func (a *AwsVpc) Init(route []*net.IPNet, kv kv.KV, kvPrefix string) error {
	sess, _ := session.NewSession(aws.NewConfig().WithMaxRetries(5))

	metadataClient := ec2metadata.New(sess)
	region, err := metadataClient.Region()
	if err != nil {
		log.Errorf("error getting EC2 region name: %v", err)
		return err
	}
	sess.Config.Region = aws.String(region)

	instanceID, err := metadataClient.GetMetadata("instance-id")
	if err != nil {
		log.Errorf("error getting EC2 instance ID: %v", err)
		return err
	}

	// Create new EC2 client
	a.ec2Svc = ec2.New(sess)

	attr := &ec2.ModifyInstanceAttributeInput{
		InstanceId:      &instanceID,
		SourceDestCheck: &ec2.AttributeBooleanValue{Value: aws.Bool(false)},
	}

	_, err = a.ec2Svc.ModifyInstanceAttribute(attr)
	if err != nil {
		log.Errorf("Error disabling src check: %v", err)
		return err
	}

	instance, err := a.ec2Svc.DescribeInstances(&ec2.DescribeInstancesInput{
		InstanceIds: []*string{aws.String(instanceID)}},
	)
	if err != nil {
		log.Errorf("error getting EC2 instance ID: %v", err)
		return err
	}

	vpcId := instance.Reservations[0].Instances[0].VpcId
	subnetID := instance.Reservations[0].Instances[0].SubnetId
	a.eniId = *instance.Reservations[0].Instances[0].NetworkInterfaces[0].NetworkInterfaceId

	log.Debugf("vpc for this instance is: %s", *vpcId)
	log.Debugf("subnetID for this instance is: %s", *subnetID)

	mainFilter := ec2.Filter{
		Name:   aws.String("association.main"),
		Values: []*string{aws.String("true")},
	}

	// TODO: filter on vpc-id

	routeTablesInput := &ec2.DescribeRouteTablesInput{
		Filters: []*ec2.Filter{&mainFilter},
	}
	res, err := a.ec2Svc.DescribeRouteTables(routeTablesInput)
	if err != nil {
		log.Errorf("error describing routeTables for subnetID %s: %v", *subnetID, err)
		return err
	}

	if len(res.RouteTables) == 0 {
		log.Errorf("routing table not found")
		return errors.New("routeing table not found")
	}

	a.routeTableId = *res.RouteTables[0].RouteTableId
	log.Debugf("routeId for this instance is: %s", a.routeTableId)

	a.route = route

	for _, r := range route {
		if err := a.createRoute(r); err != nil {
			fmt.Errorf("Unable to create route for %s", r.String())
			return err
		}
	}

	kv.Put(kvPrefix+AmsRouteTableId, a.routeTableId)

	return nil
}

func (a *AwsVpc) createRoute(net *net.IPNet) error {
	route := &ec2.CreateRouteInput{
		RouteTableId:         &a.routeTableId,
		NetworkInterfaceId:   &a.eniId,
		DestinationCidrBlock: aws.String(net.String()),
	}
	if _, err := a.ec2Svc.CreateRoute(route); err != nil {
		log.Errorf("error creating routing table %v", err)
		return err
	}

	return nil
}

func (a *AwsVpc) deleteRoute(net *net.IPNet) error {
	route := &ec2.DeleteRouteInput{
		RouteTableId:         &a.routeTableId,
		DestinationCidrBlock: aws.String(net.String()),
	}
	if _, err := a.ec2Svc.DeleteRoute(route); err != nil {
		log.Errorf("error delete routing table %v", err)
		return err
	}

	return nil
}

func (a *AwsVpc) Clean() error {
	for _, r := range a.route {
		if err := a.deleteRoute(r); err != nil {
			fmt.Errorf("Unable to delete route for %s", r.String())
		}
	}
	return nil
}

func (a *AwsVpc) UpdateKV(key string, value string) error {
	switch key {
	case AmsRouteTableId:
		a.routeTableId = value
	}

	return nil
}

func (a *AwsVpc) GetGateway() (net.IP, error) {
	routes, _ := netlink.RouteList(nil, netlink.FAMILY_V4)
	for _, a := range routes {
		if a.Dst == nil {
			return a.Gw, nil
		}
	}
	return net.IP{}, nil
}

func (a *AwsVpc) IsCompatible(vpc vpc.Vpc) bool {
	n, ok := vpc.(*AwsVpc)
	if !ok {
		return false
	}

	if a.routeTableId == "" || n.routeTableId == "" {
		return false
	}

	return a.routeTableId == n.routeTableId
}
