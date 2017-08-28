# Communication Module
Communication module is a loadable kernel module; it helps in relaying data between user space processes/services using netlink sockets.
The module requires user space process/service to send service information as registration token. Once registered, the communication module can send data to user space process/service.
The communication module forward data based on resource identifier, contained in the message. A user space process/service functionality scope is identified via defined signature. If the user space process/service signature doesn't match to known signatures, the message will be dropped in communication module. There can't two or more user space process/service of same signature.

# Build
  - `make clean` will remove object file(s)
  - `make` will compile the communication module. The module executable is placed in same folder

# Execute
  - `sudo insmod com_chan.ko` will install the communication module
  - `sudo rmmod com_chan` will remove the communication module

### Todos
  - Extend communication module to use linked list to store user space process/service information
  - Extend communication module to use service discovery setup; the purpose is to register itself with communication module(s) running in LAN
  - Extend communication module to query active communication module for resource information on respective host
  - Extend communication module to send host information along with resource data
  - Extend communiation module to use secure handshake protocol for user space process/service registration; the data communication channel between kernel space module and user space module shall also be secure. The rationale behind the requirement is to add security against bots

License
----
GPL::
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
