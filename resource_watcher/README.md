# Resource Watcher Module
Resource watcher module is a user space module; it receives queries for resource (disk information, memory information) from kernel module (communication module).
Resource watcher module registers its process/service with kernel module using defined signature. The module respond with resource information to kernel module when queried.

# Build
  - `make clean` will remove object file(s)
  - `make` will compile the module. The module executable is placed in bin directory while object files are placed in obj folder

# Execute
  - `rwatcher_1.0`

### Todos
  - Extend module to use user arguments for configurable parameter(s) e.g. encryption/encoding type for communication (when supported)
  - Extend module to register and handle process signal handler; the rationale is to signal module to quit cleanly
  - Extend module to send de-register command to kernel module before exiting
  - Extend module to handle host information along with resource information
  - Extend module to use secure handshake protocol for registration; the data communication channel between kernel space module and user space module shall also be secure. The rationale behind the requirement is to add security against bots

License
-------
GPL::
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
