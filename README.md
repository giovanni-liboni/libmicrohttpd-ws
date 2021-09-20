<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]

<!-- PROJECT LOGO -->
<br />
<p align="center">
  <h3 align="center">Libmicrohttpd WS</h3>
  <p align="center">
    An extension of the GNU Libmicrohttpd library for Websocket support
  </p>
</p>



<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgements">Acknowledgements</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

This is a modified version of libmicrohttpd (0.9.26) to support the [websocket protocol](https://en.wikipedia.org/wiki/WebSocket) natively. It partially implements the websocket protocol as specified in the technical document [rfc6455](https://datatracker.ietf.org/doc/html/rfc6455) to deploy a simple server that supports Synchronous and asynchronous text requests.

<!-- GETTING STARTED -->
## Getting Started

This is an example of how you may give instructions on setting up your project locally.
To get a local copy up and running follow these simple example steps.

### Prerequisites

Let's install the dependencies, tested on Ubuntu 18.04:

*Ubuntu 18.04
  ```sh
  sudo apt-get install cmake autoconf automake libtool libgcrypt11-dev libcurl4-gnutls-dev
  ```

### Installation

1. Get a free API Key at [https://example.com](https://example.com)
2. Create the build directory and move in
   ```sh
   mkdir build
   cd build
   ```
3. Initialize the project
   ```sh
   cmake ..
   ```
4. Compile the libmicrohttpd library
   ```sh
   make microhttpd_install
   ```

<!-- USAGE EXAMPLES -->
## Usage

**Important**: Tested with Opera, Chrome, and Brave. Currently, it does not work with Firefox.

### Example: Echo server

1. Initialize and compile the library as indicated in the **Installation** section. Then, in the build directory, run
```sh
make run_main
```

Then go to [localhost:9090](http://localhost:9090)

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<!-- LICENSE -->
## License

Distributed under the LGPLv3. See `LICENSE` for more information.

<!-- CONTACT -->
## Contact

Giovanni Liboni - giovanni@liboni.me

Project Link: [https://github.com/giovanni-liboni/libmicrohttpd-ws](https://github.com/giovanni-liboni/libmicrohttpd-ws)

<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements
* [GNU Libmicrohttpd](http://www.gnu.org/software/libmicrohttpd)
* [Best-README-Template](https://github.com/othneildrew/Best-README-Template/blob/master/README.md)
* [Choose an Open Source License](https://choosealicense.com)


<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/giovanni-liboni/libmicrohttpd-ws.svg?style=for-the-badge
[contributors-url]: https://github.com/giovanni-liboni/libmicrohttpd-ws/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/giovanni-liboni/libmicrohttpd-ws.svg?style=for-the-badge
[forks-url]: https://github.com/giovanni-liboni/libmicrohttpd-ws/network/members
[stars-shield]: https://img.shields.io/github/stars/giovanni-liboni/libmicrohttpd-ws.svg?style=for-the-badge
[stars-url]: https://github.com/giovanni-liboni/libmicrohttpd-ws/stargazers
[issues-shield]: https://img.shields.io/github/issues/giovanni-liboni/libmicrohttpd-ws.svg?style=for-the-badge
[issues-url]: https://github.com/giovanni-liboni/libmicrohttpd-ws/issues
[license-shield]: https://img.shields.io/github/license/giovanni-liboni/libmicrohttpd-ws.svg?style=for-the-badge
[license-url]: https://github.com/giovanni-liboni/libmicrohttpd-ws/blob/master/LICENSE.LGPL

