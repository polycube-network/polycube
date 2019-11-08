# Ansible playbook
An ansible playbook to install a simple k8s cluster with a master node and two worker nodes. The CNI plugin is pcn-k8s. Vagrant is used to provide the VMs.

## Requirements
 * Vagrant 2.2.5 or higher
 * Ansible 2.8.5 or higher

## Usage
 * `ansible-playbook --ask-become-pass master-playbook.yml`
 * Connecting to a node: `ssh vagrant@k8s-master-1`. 

## Configuration
All the configuration variable can be found in *kubernetes-playbooks/group_vars/all/vars.yml*. Before playing the playbook have a look at the vars.yml file and change the variables according to your needs. Please pay attention to the **Vagrant Configuration Section** and modify the variables in such a way that suits the host's network interfaces and resources such RAM and CPUs. 
By default the playbook checks if ssh key named *vagrant_machines* exists in *~/.ssh*. If it does exist than is copied in the VMs and used to login during the playbook execution and by the user to access via SSH the k8s nodes. If it does not exist than a new ssh key is generated and used to configure the VMs.
If you want to change a parameter on the fly without modifying it in the file *kubernetes-playbooks/group_vars/all/vars.yml* you can pass it to the **ansible-playbook** command like this: `ansible-playbook --ask-become-pass  --extra-vars '{"k8s_worker_nodes_ips": [192.168.0.24]}' master-playbook.yml`

## How does it work?
The first playbook to be executed is *bootstrap.yml* which does the following:
* Reads the variables in *kubernetes-playbooks/group_vars/all/vars.yml*
* Generates the **Vagrantfile** from the *kubernetes-playbooks/roles/bootstrap/templates/VagrantFile.j2*
* Generates the **kubernetes-playbooks/inventory.ini** from the *kubernetes-playbooks/roles/bootstrap/templates/inventory.ini.j2*
* For each IP in **k8s_worker_nodes_ips** generates a new file in *kubernetes-playbooks/host_vars* using the **k8s_worker_node_prefix** and populates it with an ip taken from the list.
* For each node to be part of the k8s-cluster it adds the hostname and IP address associated to that host in */etc/hosts* in the localhost filesystem.
* At the end it runs the vagrant program to provision the VMs.

Then *vagrant-netconfig*, *k8s-master-node* and *k8s-worker-node* playbooks are played. Their names are self-explanatory on what the do.

 

## Adding additional worker-nodes
If more worker-nodes are needed:
* add the ip address of the node in group_vars/all/vars.yml

## Remote user 
The remote user which runs the ansible commands is defined in two places:
* ansible.cfg
* group_vars/all/vars.yml
