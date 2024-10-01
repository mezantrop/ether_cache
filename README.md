# Ethernet messaging using raw-sockets in Linux

## Description

Ethernet cache (EC) - very basic, for education purposes, implementation of message caching `Server` and `Client` that work over Ethernet using raw-sockets in Linux.
`Client` sends a request with a `command` and a `message` with an `ID` from 0 to 127, and the `server` stores or returns a `message` depending on the `command`: `save` or, respectively,  `retrieve` it.

## Build

`make`

## Usage

Client:

```sh
Usage:
        client SRC_NIC MAC_DST 0-127 'Message to send'
        client SRC_NIC MAC_DST 0-127

For example, to send:
        client enp0s3 22:22:22:22:22:22 34 'foo bar baz bam'
And to retrieve:
        client enp0s3 22:22:22:22:22:22 56

```

Server:

```sh
Usage:
        server NIC

Example:
        server enp0s3
```

## Implementation notes

* `EC` uses 802.3, also known as historical Novell raw, frames. Using this non-common frame type allows to cut off Ethernet II noise on the network interfaces without need of MAC address filtering

* Fixed-size frames are always used, that limits effective `message` length to a maximum of 1493 bytes. Zero padding is applied for shorter messages. The same technique refers to the message buffer on the server

* On `retrieve` command, `server` always replies with a `message`. If the message is empty, e.g. ID has never been used before, the message contains only padding

* If `client` sends only padding with the `save` command, the server "clears" the slot with the specified ID

* For the sake of simplicity, as there is just a small number of IDs possible (0-127), the `server` maintains a fixed size array of strings to save messages, where ID of a message is an element index of the array

* Written on Ubuntu 24.04.1 LTS

### Format of the frame payload

* 1-st byte:
  * 1 bit    command;        0 - save message; 1 - retrieve message
  * 7 bits   message ID;
* 2-1493 bytes:
  * Zero-padded message

## Contacts

I would appreciate any feedback, additions, fixes and suggestions, sincerely yours Mikhail Zakharov <zmey20000@yahoo.com>
