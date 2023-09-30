
# Webserv: A High-Performance Web Server in C++

## Overview

Webserv is a powerful, open-source web server written in C++ that aims to recreate the functionality of industry-standard web servers like Nginx and Apache. Whether you're looking to host a simple website or need a robust server for a complex web application, Webserv is here to meet your needs.

Our project is designed to be both user-friendly and highly configurable. In this README, we'll guide you through the process of compiling and running Webserv. We'll also provide comprehensive documentation for the configuration file format, allowing you to customize your server's behavior to suit your specific requirements.

## Table of Contents

- [Webserv: A High-Performance Web Server in C++](#webserv-a-high-performance-web-server-in-c)
	- [Overview](#overview)
	- [Table of Contents](#table-of-contents)
	- [Getting Started](#getting-started)
	- [Compiling Webserv](#compiling-webserv)
	- [Running Webserv](#running-webserv)
	- [Configuration File](#configuration-file)
		- [Grammar](#grammar)
		- [Sample Configuration](#sample-configuration)
		- [Configuration Options](#configuration-options)
	- [Resources](#resources)
	- [License](#license)

## Getting Started

To begin using Webserv, follow these simple steps:

1.  Clone the Webserv repository to your local machine:
    
	```shell
	git clone https://github.com/Anarbb/webserv.git
	```
    
2.   Navigate to the project directory:
		```shell
		cd webserv
		```


## Compiling Webserv

Webserv can be compiled with a simple `make` command. This will generate the executable binary in the `./bin` directory:

```shell
make
```


## Running Webserv

You can start Webserv with or without specifying a configuration file. If you don't provide a configuration file, Webserv will use a default configuration.

-   Start Webserv with a specific configuration file:
    
	```shell
	./bin/webserv <config_file>
	```
    
-   Start Webserv with the default configuration:
    
	```shell
	./bin/webserv
	```
    

Now, let's explore the configuration file format and options.

## Configuration File

Webserv's configuration file allows you to define various server settings to customize its behavior. Below, we provide detailed documentation for the configuration file format.

### Grammar

The configuration file follows this grammar:

```plaintext
<config> := <server> | <server> <config>

<server> := "server" "{" "\n" <server_body> "}" "\n"

<server_body> := <listen> | <server_name> | <host> | <error_page> | <client_body_size> | <root> | <location>

<listen> := "listen:" NUMBER "\n"

<server_name> := "server_name:" WORD "\n"

<host> := "host:" IP_ADDRESS "\n"

<error_page> := "error_page:" NUMBER "," WORD "\n"

<client_body_size> := "client_body_size:" NUMBER "\n"

<root> := "root:" WORD "\n"

<location> := "location" WORD "{" "\n" <location_body> "}" "\n"

<location_body> := <allow_method> | <autoindex> | <index> | <compiler> | <return>

<allow_method> := "allow_methods:" <methods> "\n"

<methods> := <method> | <method> "," <methods>

<method> := "POST" | "GET" | "DELETE"

<autoindex> := "autoindex:" ("on" | "off") "\n"

<index> := "index:" WORD "\n"

<compiler> := "compiler:" WORD "\n"

<return> := "return:" WORD "\n"` 
```
### Sample Configuration

Here's a sample configuration file to help you get started:
```yaml
server {
    host: "0.0.0.0"
    listen: "80"
    root: "./root"
    client_body_size: "600000000"
    allow_methods: "GET","POST","DELETE"
    upload_path: "./upload"
    error_page: "404","./errors/404.html","500","./errors/404.html"
    index: "index.html"
    server_name: "main"
    location "/cgi" {
        allow_methods: "GET","POST"
        root: "./root/cgi-bin"
        autoindex: "on"
    }
    location "/msmen" {
        allow_methods: "GET"
        root: "./root/msmen"
        index: "index.html"
    }
    location "/rick" {
        return: "https://www.youtube.com/watch?v=8ybW48rKBME"
    }
     location "*.py" {
        allow_methods: "GET","POST"
        root: "./root/cgi-bin/"
        compiler: "/usr/bin/python3"
    }
    location "*.rb" {
        allow_methods: "GET","POST"
        root: "./root/cgi-bin"
        compiler: "/usr/bin/ruby"
    }
}
```
### Configuration Options

-   `server`: Defines the server block.
    
-   `host`: Specifies the host IP address for the server.
    
-   `listen`: Defines the port to listen on.
    
-   `root`: Specifies the root directory for serving web content.
    
-   `client_body_size`: Sets the maximum allowed client request body size.
    
-   `allow_methods`: Defines the HTTP methods allowed on the server.
    
-   `upload_path`: Specifies the path for handling uploaded files.
    
-   `error_page`: Configures custom error pages.
    
-   `server_name`: Specifies the server name.
    
-   `location`: Defines location blocks for specific paths.
    
-   `autoindex`: Enables or disables directory listing.
    
-   `index`: Specifies the default index file.
    
-   `compiler`: Defines the path to the compiler for specific file types.
    
-   `return`: Redirects requests to a different URL.
    
## Resources

  

- https://datatracker.ietf.org/doc/html/rfc2616#section-1

- https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa

- https://www3.ntu.edu.sg/home/ehchua/programming/webprogramming/http_basics.html

- https://beej.us/guide/bgnet/html/split/

- https://notes.shichao.io/unp/

- https://developer.mozilla.org/en-US/

## License

This project is licensed under the GNU General Public License (GPL). See the [LICENSE](LICENSE) file for details.
