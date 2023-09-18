#include "Response.hpp"
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
Response::Response() :
      _protocol(""),
      _status_code(""),
      _body(""),
      _headers()
{
    _dataSent = 0;
    _fd = 0;
    _offset = 0;
    _isCGI = false;
    _filePath = "";
    _buffer = "";
    _clientSocket = -1;
    _headersSent = false;
}

Response::Response(int clientSocket)
    : _protocol(""),
      _status_code(""),
      _body(""),
      _headers()
{
    _dataSent = 0;
    _fd = 0;
    _offset = 0;
    _isCGI = false;
    _filePath = "";
    _buffer = "";
    _clientSocket = clientSocket;
    _headersSent = false;
}

Response::Response(int clientSocket, std::vector<ServerConf> &servers)
    : _protocol(""),
      _status_code(""),
      _body(""),
      _headers()
{
    _dataSent = 0;
    _fd = 0;
    _offset = 0;
    _isCGI = false;
    _filePath = "";
    _buffer = "";
    _clientSocket = clientSocket;
    _headersSent = false;
    _servers = servers;
    _error = 0;
    _autoindex = false;
    _isTextStream = false;
    _redirect = "";
}

Response::~Response()
{
}

void Response::findStatusCode(Request &req)
{
    unused(req);
    _status_code = "200 OK";
    switch (_error)
    {
    case 301:
        _status_code = "301 Moved Permanently";
        break;
    case 404:
        _status_code = "404 Not Found";
        break;
    case 405:
        _status_code = "405 Method Not Allowed";
        break;
    case 500:
        _status_code = "500 Internal Server Error";
        break;
    case 501:
        _status_code = "501 Not Implemented";
        break;
    case 505:
        _status_code = "505 HTTP Version Not Supported";
        break;
    default:
        break;
    }
}

int Response::findRouting(Request &req)
{
    std::vector<Location> &locations = _config.location;

    for (std::vector<Location>::iterator it = locations.begin(); it != locations.end(); it++)
    {
        if (req.getPath().find(it->getLocationName()) == 0)
        {
            _location = *it;
            _root = it->getString(ROOT);
            _index = it->getString(INDEX);
            _autoindex = it->getAutoindex();
            _errorPages = it->getErrorPages();
            _methods = it->getMethods();
            _redirect = it->getReturned();
            // remove location name from path
            int idx = req.getPath().find(it->getLocationName());
            std::string locationName = it->getLocationName();
            std::string relativePath = req.getPath().find_first_of(locationName) == 0 ? req.getPath().substr(locationName.length()) : req.getPath().substr(0, idx);
            _filePath = constructFilePath(relativePath, _root, _index);
            
            if (_redirect.empty() == false && (req.getPath().substr(it->getLocationName().length()) == "/" || req.getPath().substr(it->getLocationName().length()) == "") && req.getMethod() == "GET")
            {
                _error = 301;
                return _error;
            }
            if (_autoindex == true)
            {
                _filePath = constructFilePath(relativePath, _root, "");
                _isTextStream = true;
                std::cout << _filePath << std::endl;
                return CONTINUE;
            }
            return CONTINUE;
        }
    }
    _root = _config.getString(ROOT);
    _index = _config.getString(INDEX);
    _autoindex = _config.getAutoindex();
    _errorPages = _config.getErrorPages();
    _methods = _config.getMethods();
    _filePath = constructFilePath(req.getPath(), _root, _index);
    if (_autoindex == true)
    {
        _filePath = constructFilePath(req.getPath(), _root, "");
        _isTextStream = true;
        return CONTINUE;
    }
    else
    {
        _filePath = constructFilePath(req.getPath(), _root, _index);
        _isTextStream = false;
    }
    return CONTINUE;
}

void Response::handleDefaultError(Request &req)
{
    unused(req);
    std::stringstream ss;
    ss << "<center><h1>" << _error << " Error</h1></center>";
    _body = ss.str();
    _fileSize = _body.size();
    _headers["Content-Type"] = "text/html";
    _isTextStream = true;
}

void Response::handleError(Request &req)
{
    if (_error == 301)
    {
        _fileSize = 0;
        _isTextStream = true;
    }
    else if (_error == 404 || _error == 405 || _error == 501 || _error == 500 || _error == 505)
    {
        if (_errorPages.empty())
        {
            _errorPages = _config.getErrorPages();
            if (_errorPages.empty())
            {
                handleDefaultError(req);
                return;
            }
        }
        _filePath = constructFilePath(_errorPages[_error], _root, _index);
        std::cout << "Error file path: " << _filePath << std::endl;
        _fd = open(_filePath.c_str(), O_RDONLY);
    }
}

std::string findDirname(std::string &path, std::string &root)
{
    // remove root from path and return the dirname
    std::string dirname = path.substr(root.length());
    size_t pos = dirname.find_last_of('/');
    if (pos == std::string::npos)
        return "";
    dirname = dirname.substr(0, pos);
    return dirname;
}


void Response::genListing()
{
    std::string path = _filePath;
    DIR *dir = opendir(path.c_str());
    struct dirent *ent;
    if (dir == NULL)
    {
        _error = 69;
        return;
    }
    std::stringstream ss;
    std::string location = _location.getLocationName();
    std::string pathName;
    if (!location.empty())
        pathName = location + "/";
    else
        pathName = findDirname(path, _root) + "/";
    if (pathName.empty())
        ss << "<html><head><title>Index of " << "/" << "</title></head><body bgcolor=\"white\"><h1>Index of " << "/" << "</h1><hr><pre>";
    else
        ss << "<html><head><title>Index of " << pathName << "</title></head><body bgcolor=\"white\"><h1>Index of " << pathName << "</h1><hr><pre>";

    if ((dir = opendir(path.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_name[0] == '.' && !ent->d_name[1])
                continue;
            ss << "<a href=\"" << pathName << ent->d_name << "\">" << ent->d_name << "</a><br>";
        }
        closedir(dir);
    }
    ss << "</pre><hr></body></html>";
    _body = ss.str();
    _fileSize = _body.size();
    _isTextStream = true;
}

void Response::InitFile(Request &req)
{
    int routing = findRouting(req);
    if (routing == 404)
    {
        handleError(req);
        if (_fd == -1)
        {
            handleDefaultError(req);
            return;
        }
        return;
    }
    else if (routing == 301)
    {
        handleError(req);
        return;
    }
    if (_autoindex == true)
    {
        if (_isTextStream == true)
            genListing();
        if (_error != 69)
        {
            return;
        }
        _isTextStream = false;
        _error = 0;
    }
    if (isDirectory(_filePath.c_str()))
    {
        _error = 404;
        handleError(req);
        if (_fd == -1)
        {
            handleDefaultError(req);
            return;
        }
        return;
    }
    _fd = open(_filePath.c_str(), O_RDONLY);
    if (_fd == -1)
    {
        _error = 404;
        handleError(req);
        if (_fd == -1)
        {
            handleDefaultError(req);
            return;
        }
    }
    _fileSize = lseek(_fd, 0, SEEK_END);
    lseek(_fd, 0, SEEK_SET);
}

void Response::checkMethod(Request &req)
{
    std::string const &method = req.getMethod();
    if (_methods.empty())
    {
        _methods.push_back("GET");
        _methods.push_back("POST");
        _methods.push_back("DELETE");
    }
    for (std::vector<std::string>::iterator it = _methods.begin(); it != _methods.end(); it++)
    {
        if (*it == method)
            return;
    }
    _error = 405;
    handleError(req);
}

void Response::checkHttpVersion(Request &req)
{
    std::string const &version = req.getHTTPVersion();
    if (version != "HTTP/1.1")
    {
        _error = 505;
        handleError(req);
    }
}

void Response::InitHeaders(Request &req)
{
    std::stringstream ss;
    checkHttpVersion(req);
    checkMethod(req);
    // checkCGI(req);
    findStatusCode(req);
    _headers["Server"] = "Webserv/1.0";
    if (_headers.find("Content-Type") == _headers.end())
    {
        _headers["Content-Type"] = getContentType(_filePath);
    }
    if (_isTextStream == true && _redirect.empty())
    {
        ss << _body.length();
        _headers["Content-Length"] = ss.str();
        _headers["Connection"] = "keep-alive";
        return;
    }
    if (!_redirect.empty())
    {
        _headers["Location"] = _redirect;
    }
    ss << _fileSize;
    _headers["Content-Length"] = ss.str();
    _headers["Connection"] = "keep-alive";
}

#ifdef __APPLE__
int Response::sendFileData()
{
    if (_headersSent == false && _isCGI == true)
    {
        _fileSize = lseek(_fd, 0, SEEK_END);
        lseek(_fd, 0, SEEK_SET);
        std::string body ="HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nServer: Webserv/1.0\r\nContent-Length: " + std::to_string(_fileSize) + "\r\n\r\n";
        ssize_t bytesSent = send(_clientSocket, body.c_str(), strlen(body.c_str()), 0);
     if (bytesSent == -1)
     {
         std::cout << "error" << std::endl;
         return ERROR;
     }
        _headersSent = true;
    }
    off_t bytesSent = 1024;
    int res = sendfile(_fd, _clientSocket, _offset, &bytesSent, NULL, 0);
    if (res == -1 && _offset >= _fileSize)
    {
        std::cout << "error" << std::endl;
        close(_fd);
        return DONE;
    }
    _offset += bytesSent;
    if (_offset >= _fileSize)
    {
        if (_fd > 0)
        {
        close(_fd);
        return DONE;
        }
    }
    return CONTINUE;
}
#else
#include <sys/sendfile.h>

int Response::sendFileData()
{
    off_t offset = _offset; // Save the offset before modifying it
    off_t remainingBytes = _fileSize - offset;

    ssize_t bytesSent = sendfile(_clientSocket, _fd, &offset, remainingBytes);
    if (bytesSent == -1)
    {
        return ERROR;
    }
    else if (bytesSent == 0 && offset >= _fileSize)
    {
        close(_fd);
        _fd = -1;
        _offset = 0;
        return DONE;
    }

    _offset = offset;
    if (_offset >= _fileSize)
    {
        close(_fd);
        _fd = -1;
        _offset = 0;
        return DONE;
    }
    return CONTINUE;
}
#endif

int Response::sendTextData()
{
    std::string &body = _body;
    ssize_t remainingBytes = body.length() - _dataSent;
    ssize_t bytesSent = send(_clientSocket, body.c_str() + _dataSent, remainingBytes, 0);
    _dataSent += bytesSent;
    if (bytesSent == -1)
    {
        return ERROR;
    }
    else if (bytesSent == 0 && _dataSent >= body.length())
    {
        _dataSent = 0;
        return DONE;
    }
    _dataSent += bytesSent;
    if (_dataSent >= body.length())
    {
        _dataSent = 0;
        return DONE;
    }
    return CONTINUE;
}

void    Response::findConfig(Request &req)
{
    const std::map<std::string, std::string> &headers = req.getHeaders();
    if (headers.find("Host") != headers.end())
    {
        std::string host = headers.find("Host")->second;
        for (std::vector<ServerConf>::iterator it = _servers.begin(); it != _servers.end(); it++)
        {
            if (it->getString(SERVER_NAME) == host)
            {
                _config = *it;
                return;
            }
        }
    }
    _config = _servers[0];
}

int Response::sendResp(Request &req, int fdcgi)
{
    findConfig(req);
    if (fdcgi > 0)
    {
        _fd = fdcgi;
        _isCGI = true;
        return sendFileData();
    }
    std::stringstream ss;
    if (_fd == 0 && _isCGI == false)
    {
        InitFile(req);
        InitHeaders(req);
    }
    if (_headersSent == false)
    {
        ss << "HTTP/1.1 " << _status_code << "\r\n";
        for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++)
        {
            ss << it->first << ": " << it->second << "\r\n";
        }
        ss << "\r\n";
        _buffer = ss.str();
        send(_clientSocket, _buffer.c_str(), _buffer.length(), 0);
        _headersSent = true;
    }
    if (_isTextStream)
        return sendTextData();
    else
        return sendFileData();
    // } catch (std::exception &e) {
    //     std::cout << e.what() << std::endl;
    //     return DONE;
    // }
    return CONTINUE;
}
