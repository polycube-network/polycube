#!/usr/bin/python3
# coding: utf-8

import argparse
import socket
import requests
import json
import os.path
from os import path

VERSION = '1.0'
POLYCUBED_ADDR = 'localhost'
POLYCUBED_PORT = 9000
REQUESTS_TIMEOUT = 5 #seconds

polycubed_endpoint = 'http://{}:{}/polycube/v1'


def main():
    global polycubed_endpoint
    args = parseArguments()

    addr = args['address']
    port = args['port']

    cube_name = args['cube_name']
    interface_name = args['peer_interface']
    path_to_dataplane = args['path_to_configuration']

    dataplane = None

    if not path.isfile(path_to_dataplane):
        print(f'File {path_to_dataplane} does not exist.')
        exit(1)
    else:
        with open(path_to_dataplane) as json_file:
            dataplane = json.load(json_file)

    polycubed_endpoint = polycubed_endpoint.format(addr, port)

    checkConnectionToDaemon()

    already_exists, cube = checkIfServiceExists(cube_name)

    if already_exists:
        print(f'Dynmon {cube_name} already exist')
        attached_interface = cube['parent']
        if attached_interface:
            if attached_interface != interface_name:
                detach_from_interface(cube_name, attached_interface)
                attach_to_interface(cube_name, interface_name)
        else:
            attach_to_interface(cube_name, interface_name)

        injectNewDataplane(cube_name, dataplane)

    else:
        createInstance(cube_name, dataplane)
        attach_to_interface(cube_name, interface_name)


def checkConnectionToDaemon():
    try:
        requests.get(polycubed_endpoint, timeout=REQUESTS_TIMEOUT)
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def checkIfServiceExists(cube_name):
    try:
        response = requests.get(f'{polycubed_endpoint}/dynmon/{cube_name}', timeout=REQUESTS_TIMEOUT)
        response.raise_for_status()
        return True, json.loads(response.content)
    except requests.exceptions.HTTPError:
        return False, None
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def injectNewDataplane(cube_name, dataplane):
    try:
        print('Injecting the new dataplane')
        response = requests.put(f'{polycubed_endpoint}/dynmon/{cube_name}/dataplane',
                                json.dumps(dataplane),
                                timeout=REQUESTS_TIMEOUT)
        response.raise_for_status()
    except requests.exceptions.HTTPError:
        print(f'Error: {response.content.decode("UTF-8")}')
        exit(1)
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def createInstance(cube_name, dataplane):
    try:
        print(f'Creating new dynmon instance named {cube_name}')
        response = requests.put(f'{polycubed_endpoint}/dynmon/{cube_name}',
                                json.dumps({'dataplane': dataplane}),
                                timeout=REQUESTS_TIMEOUT)
        response.raise_for_status()
    except requests.exceptions.HTTPError:
        print(f'Error: {response.content.decode("UTF-8")}')
        exit(1)
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def detach_from_interface(cube_name, interface):
    try:
        print(f'Detaching {cube_name} from {interface}')
        response = requests.post(f'{polycubed_endpoint}/detach',
                                 json.dumps({'cube': cube_name, 'port': interface}),
                                 timeout=REQUESTS_TIMEOUT)
        response.raise_for_status()
    except requests.exceptions.HTTPError:
        print(f'Error: {response.content.decode("UTF-8")}')
        exit(1)
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def attach_to_interface(cube_name, interface):
    try:
        print(f'Attaching {cube_name} to {interface}')
        response = requests.post(f'{polycubed_endpoint}/attach',
                                 json.dumps({'cube': cube_name, 'port': interface}),
                                 timeout=REQUESTS_TIMEOUT)
        response.raise_for_status()
    except requests.exceptions.HTTPError:
        print(f'Error: {response.content.decode("UTF-8")}')
        exit(1)
    except requests.exceptions.ConnectionError:
        print('Connection error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.Timeout:
        print('Timeout error: unable to connect to polycube daemon.')
        exit(1)
    except requests.exceptions.RequestException:
        print('Error: unable to connect to polycube daemon.')
        exit(1)


def parseArguments():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('cube_name', help='indicates the name of the cube', type=str)
    parser.add_argument('peer_interface', help='indicates the network interface to connect the cube to', type=str)
    parser.add_argument('path_to_configuration',
                        help='''indicates the path to the json file which contains the new dataplane configuration
                                which contains the new dataplane code and the metadata associated to the exported metrics''',
                        type=str)
    parser.add_argument('-a', '--address', help='set the polycube daemon ip address', type=str, default=POLYCUBED_ADDR)
    parser.add_argument('-p', '--port', help='set the polycube daemon port', type=int, default=POLYCUBED_PORT)
    parser.add_argument('-v', '--version', action='version', version=showVersion())
    return parser.parse_args().__dict__


def showVersion():
    return '%(prog)s - Version ' + VERSION


if __name__ == '__main__':
    main()
