# import modules
import struct
import time
import binascii
import sys
import os
import json
import requests
import ipaddress
import logging
import threading
import time
from requests.exceptions import HTTPError

# Global Header Values
PCAP_MAGICAL_NUMBER = 0
PCAP_MJ_VERN_NUMBER = 0
PCAP_MI_VERN_NUMBER = 0
PCAP_LOCAL_CORECTIN = 0
PCAP_ACCUR_TIMSTAMP = 0
PCAP_MAX_LENGTH_CAP = 0
PCAP_DATA_LINK_TYPE = 0
service_name = ''
service_num = 0
Nservices = 1
f = 0
stop = False


def connect(address):
    url = 'http://'+address+':9000/polycube/v1/cubes/'
    try:
        response = requests.get(url, timeout=8)
        response.raise_for_status()
    except HTTPError as http_err:
        print(f'HTTP error occurred: {http_err}')
        exit()
    except Exception as err:
        print(f'Other error occurred: {err}')
        exit()
    else:
        if response.text == "{}":
            print('\t=== No packetcapture services up ===')
            exit()
        print('\t===Packetcapture Client===\n\n')
        return json.loads(response.text)


def printnames(todos):
    global Nservices
    print('services:\n')
    for pktcap in todos['packetcapture']:
        print(str(Nservices) + ' -> ' + pktcap['name'])
        Nservices+=1
    Nservices-=1
    print()
    

def selectService(todos, address):
    global service_num, Nservices, service_name
    try:
        service_num = int(input('Please select the service number: '))
        if service_num > Nservices or service_num < 1:
            exit()
        service_name = todos['packetcapture'][service_num-1]['name']
        url = 'http://'+address+':9000/polycube/v1/packetcapture/'+service_name+'/'
        try:
            response = requests.get(url, timeout=8)
            response.raise_for_status()
        except HTTPError as http_err:
            print(f'HTTP error occurred: {http_err}')
            exit()
        except Exception as err:
            print(f'Other error occurred: {err}')
            exit()
        todos2 = json.loads(response.text)
        if todos2['networkmode'] is False:
            print('-----------------------------------')
            networkmode_response = input('\n\t=== The service is not in network mode ===\nDo you want to set up this service in networkmode (y / n)? ')
            if networkmode_response != 'y':
                exit()
            else:
                url = 'http://'+address+':9000/polycube/v1/'+service_name+'/networkmode/'
                requests.patch(url, 'true')
    except:
        print("Closing...")


def showusage():
    print("client.py <IPv4 address> <file destination name>")
    print("\t-h/--help: show the usage menu")


def checkIp(param):
    if param == 'localhost':
        return True
    try:
        ret = ipaddress.ip_address(param)
        if isinstance(ret,ipaddress.IPv4Address) is False:
            print("Supports only IPv4")
            exit()
    except:
        print("Invalid IP address")
        exit()


def initGlobalHeader(address):
    global service_name, PCAP_MAGICAL_NUMBER, PCAP_MJ_VERN_NUMBER, PCAP_MI_VERN_NUMBER, PCAP_LOCAL_CORECTIN, PCAP_ACCUR_TIMSTAMP, PCAP_MAX_LENGTH_CAP, PCAP_DATA_LINK_TYPE
    url = 'http://'+address+':9000/polycube/v1/packetcapture/'+service_name+'/globalheader/'
    try:
        response = requests.get(url, timeout=8)
        response.raise_for_status()
    except HTTPError as http_err:
        print(f'HTTP error occurred: {http_err}')
        exit()
    except Exception as err:
        print(f'Other error occurred: {err}')
        exit()
    todos3 = json.loads(response.text)
    PCAP_MAGICAL_NUMBER = todos3['magic']
    PCAP_MJ_VERN_NUMBER = todos3['version_major']
    PCAP_MI_VERN_NUMBER = todos3['version_minor']
    PCAP_LOCAL_CORECTIN = todos3['thiszone']
    PCAP_ACCUR_TIMSTAMP = todos3['sigfigs']
    PCAP_MAX_LENGTH_CAP = todos3['snaplen']
    PCAP_DATA_LINK_TYPE = todos3['linktype']


def writeGlobalHeader():
    global f
    f = open(sys.argv[2] + ".pcap", "ab+")
    f.write(struct.pack('@ I H H i I I I ', PCAP_MAGICAL_NUMBER, PCAP_MJ_VERN_NUMBER, PCAP_MI_VERN_NUMBER, PCAP_LOCAL_CORECTIN, PCAP_ACCUR_TIMSTAMP, PCAP_MAX_LENGTH_CAP, PCAP_DATA_LINK_TYPE))
    

def getAndWritePacket(address):
    global f
    url = 'http://'+address+':9000/polycube/v1/packetcapture/'+service_name+'/packet/'
    try:
        response = requests.get(url, timeout=8)
        response.raise_for_status()
    except HTTPError as http_err:
        print(f'HTTP error occurred: {http_err}')
        exit()
    except Exception as err:
        print(f'Other error occurred: {err}')
        exit()
    todos4 = json.loads(response.text)
    if todos4['rawdata'] != "":
        f.write(struct.pack('@ I I I I ', todos4['timestamp-seconds'], todos4['timestamp-microseconds'], todos4['capturelen'], todos4['packetlen']))
        asciidata = todos4['rawdata'].encode("latin-1","strict")
        f.write(asciidata)
    else:
        time.sleep(0.1)


def listening():
    global f, stop
    while(1):
        if stop:
            f.close()
            break
        getAndWritePacket(sys.argv[1])


def main():
    global stop

    #Checking arguments
    if len(sys.argv) != 3 or sys.argv[1] == "-h" or sys.argv[1] == "--help":
        showusage()
        exit()

    checkIp(sys.argv[1])
    todos = connect(sys.argv[1])
    printnames(todos)
    selectService(todos, sys.argv[1])
    initGlobalHeader(sys.argv[1])
    writeGlobalHeader()
    listener = threading.Thread(target=listening)
    listener.start()
    while(1):
        usr_cmd = input('write "stop" to end the capture: ')
        if usr_cmd == 'stop':
            stop = True
            break
    listener.join()


if __name__ == "__main__":
    main()
