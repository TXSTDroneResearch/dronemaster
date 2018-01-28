# Drone Master

This software compliments the Roomba slaves and implements a master server that can run
on either a Bebop drone host, or a linux host (as a substitute during development).

The gist of this software (as it stands) is:

1) (`main.cc`) Initializes a TCP server running on port 1444
2) (`main.cc`) Begins an Avahi service broadcasting `_roomba._tcp`
3) (`roomba_server.cc`) Upon connection, we initialize the client and send a dummy
   command to make it rotate clockwise at full speed.

More in-depth documentation can be found under src/

## How to build

First, install these libraries:

```
sudo apt install -y cmake libavahi-client-dev libavahi-common-dev libavahi-core-dev
```

Then, create the build directory and generate the makefiles:

```
mkdir build
cd build
cmake ..
```

And then build it.

```
make -j
```

## How to Run

```
./MasterServer
```