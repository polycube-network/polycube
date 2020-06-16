#!/usr/bin/python3
# coding: utf-8

import time, argparse, requests, json, socket, os, errno
from datetime import datetime

VERSION = '1.1'
POLYCUBED_ADDR = 'localhost'
POLYCUBED_PORT = 9000
REQUESTS_TIMEOUT = 5

polycubed_endpoint = 'http://{}:{}/polycube/v1'

def main():
    global polycubed_endpoint

    args = parseArguments()

    addr = args['address']
    port = args['port']
    cube_name = args['cube_name']
    save = args['save']
    p_type = args['type']
    m_name = args['name']

    polycubed_endpoint = polycubed_endpoint.format(addr, port)


    res = getMetrics(cube_name) if p_type == 'all' else getMetrics(cube_name, p_type, m_name)

    #Removing Hateoas _links
    if m_name is not None and len(res):
        res = res[0]
    elif p_type == 'all':
        del res['_links']

    if save:
        with open(f'dump.json', 'w') as fp:
            json.dump(res, fp, indent=2)
    else:
        print(json.dumps(res, indent=2))


def getMetrics(cube_name, program_type = None, metric_name = None):
    program_type_endpoint = f'{program_type}-metrics' if program_type else ''
    metric_name_endpoint = f'{metric_name}/value' if metric_name else  ''
    try:
        response = requests.get(f'{polycubed_endpoint}/dynmon/{cube_name}/metrics/{program_type_endpoint}/{metric_name_endpoint}', timeout=REQUESTS_TIMEOUT)
        if response.status_code == 500:
            print(response.content)
            exit(1)
        response.raise_for_status()
        return json.loads(response.content)
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


def parseArguments():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('cube_name', help='indicates the name of the cube', type=str)
    parser.add_argument('-a', '--address', help='set the polycube daemon ip address', type=str, default=POLYCUBED_ADDR)
    parser.add_argument('-p', '--port', help='set the polycube daemon port', type=int, default=POLYCUBED_PORT)
    parser.add_argument('-s', '--save', help='store the retrieved metrics in a file', action='store_true')
    parser.add_argument('-t', '--type', help='specify the program type ingress/egress to retrieve the metrics', type=str, choices=['ingress', 'egress', 'all'], default='all')
    parser.add_argument('-n', '--name', help='set the name of the metric to be retrieved', type=str, default=None)
    args = parser.parse_args().__dict__
    if args['name'] and args['type'] == 'all':
        parser.error('a program --type must be specified when setting the metric --name')
    return args


if __name__ == '__main__':
    main()