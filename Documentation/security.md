# Securing the polycubed daemon

Polycube uses a security model based on X509 certificates to secure polycube daemon to polycube cli communication.

## polycubed


### Server authentication


In order to authenticate the server the ``cert`` and ``key`` parameters are needed.

Example:

```
    # polycubed configuration file
    cert: path to server certificate
    key: path to server key
```

### Client authentication


Polycubed supports thee different modes to perform client authentication.

#### Mode 1: Certification authority based


This method requires you to create own signed certificates for the clients.
Please see [Create own certification authority with openssl](#create-own-certification-authority-with-openssl) to generate the client certificates.

In this mode the following parameters are needed:

- ``cert``: server certificate
- ``key``: server key
- ``cacert``: certification authority certificate used to authenticate clients

Any client with a certificate signed by that certification authority is able to use polycubed.

Configuration example:

```

  # polycubed configuration file
  # server certificate
  cert: /home/user/server.crt
  # server private key
  key: /home/user/server.key
  # CA certificate used to authenticate clients
  cacert: /home/user/ca.crt

```

#### Mode 2: Certification Authority based with blacklist


This mode is an extension to Mode 1 that allows to deny the access to given certificates by passing the ``cert-black-list``, it is a folder containing hashed names for the banned certificates.
See [How to generate hash links to certificates](#how-to-generate-hash-links-to-certificates).

Configuration example:

```

# polycubed configuration file
cert: /home/user/server.crt
# server private key
key: /home/user/server.key
# CA certificate used to authenticate clients
cacert: /home/user/ca.crt
# folder with blacklisted certificates
cert-black-list: /home/user/my_black_list/

```

#### Mode 3: Whitelist based


This mode allows to use already existing client certificates by providing the ``cert-white-list`` parameter that is a folder containing hash named client certificates allowed to access polycubed.
See :ref:`c-rehash`.

Configuration example:

```

# polycubed configuration file
cert: /home/user/server.crt
# server private key
key: /home/user/server.key
# folder with allowed certificates
cert-white-list: /home/user/my_white_list/

```

## polycubectl


To enable a secure connection to polycubed the user has configure the following parameters for polycubectl.
See :ref:`polycubectl configuration <polycubectl-configuration>` to get more details.

- ``url``: must start with ``https``
- ``cert``: client certificate
- ``key``: client private key
- ``cacert``: certification authority certificate that signed the server certificate


## Create own certification authority with openssl


You can create your own certification authority to issue certificates for polycubed and the clients.

1. Create certification authority

    a. Create root key
    ```
        openssl genrsa -des3 -out ca.key 4096
    ```

    Remove the ``-des3`` if you don't have to protect the key wiht a passphrase

    b. Create root certificate
    ```
        openssl req -x509 -new -nodes -key ca.key -sha256 -days 1024 -out ca.crt
    ```
2. Create polycubed certificate

    This step can be skipped if you already have a valid certificate to be used.

    a. Create polycubed private key
    ```
        openssl genrsa -out server.key 2048
    ```
    b. Create certificate request for polycubed
    ```
        openssl req -new -key server.key -out server.csr
    ```
    c. Generate server certificate

    The server certificate must have the alternative name set to the IP or domain where polycubed will run

    Create a server.conf file with the following content. Add the DNS entries you need.
    ```
        [req_ext]
        subjectAltName = @alt_names
        [alt_names]
        DNS.1 = localhost
        DNS.2 = 127.0.0.1
    ```
    Create certificate
    ```
        openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
            -out server.crt -days 1024 -sha256 -extfile server.conf -extensions req_ext
    ```
3. Create client certificate

    a. Create client key
    ```
        openssl genrsa -out client.key 2048
    ```
    b. Create client certificate request
    ```
        openssl req -new -key client.key -out client.csr
    ```
    c. Generate client certificate
    ```
        openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
            -out client.crt -days 1024 -sha256
    ```

    Please keep a copy of the client certificates you generate, they could be uselful in the future in case you want to use the ``cert-black-list`` option.

## How to generate hash links to certificates


The ``cert-black-list`` and ``cert-white-list`` parameters refer to a folder that contains certificates named by their hash value.

Follow these instructions to generate hash links to certificates:

```
# copy the certificates to your black or whitelist folder
$ cp client.crt myfolder/
$ cd myfolder
$ ls
client.crt
# create symbolic links
$ c_rehash .
Doing .
$ ls -l
9d75b5b3.0 -> client.crt
client1.crt
eb7bf4cd.0 -> client.crt
```

Please see the [c_rehash](https://www.openssl.org/docs/man1.0.2/apps/c_rehash.html) tool to get more information.