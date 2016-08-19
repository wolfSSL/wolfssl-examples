## Introduction

This project provides a GUI for the wolfCrypt benchmark located in the main wolfSSL repository.  It is built on top of node.js and nw.js.

## Build Instructions
#### Dependencies

* wolfSSL
* git
* Node.js & npm
* Bower

1. Download and install node.js from nodejs.org.
  * Tested with node.js v6.3.0
2. Download and install git from https://git-scm.com/downloads or preferred package manager.
3. Open command prompt or terminal.
4. Install bower globally with `npm install bower -g`
  * `npm` command should be installed by node.js.
  * If `npm` command is not found, restart terminal.
  * If `npm` command is still not found, reinstall node.js.

#### Build

```
$ git clone https://github.com/wolfssl/wolfssl-examples
$ cd wolfssl-examples/benchmarkgui
$ npm install
$ bower install
```

`npm install` will install nw.js and its dependencies.  `bower install` will install client side dependencies for the application like d3, and boostrap.

#### Run

Before this project can be ran, wolfSSL must be downloaded and compiled.  A symlink to the wolfCrypt benchmark must be made in the root of this project.

```
$ npm start
```

`npm start` runs the start script defined in `package.json`.

## License

benchmarkGUI is licensed under the GNU GPL v2.  This license applies to all files in this directory except for /js/d3gauge.js.

Copyright (C) 2006-2016 wolfSSL Inc.

benchmarkGUI is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

benchmarkGUI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 
