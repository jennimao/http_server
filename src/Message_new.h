#include <string>
enum MethodType {
    GET,
    POST,
    NO_METHOD
};

class RequestTarget {
    std::string requestTarget;
    public:
    RequestTarget(){
    }
    void setTarget(std::string newTarget) {
        requestTarget = newTarget;
    }
    std::string getTarget(void) {
        return requestTarget;
    }
};

enum Version {
    HTTP_1_1,
    NO_VERSION
};

class VirtualHost {
    std::string virtualHost;
   
    public:

    void setTarget(std::string newHost) {
        virtualHost = newHost;
    }
};

enum MediaType{
    TEXT,
    WILDCARD,
    HTML,
    IMAGE,
    PNG,
    JPEG,
    NONE
};

class AcceptHeader {
    std::string raw_header;
    MediaType media_category = MediaType::NONE;
    MediaType media__sub_category = MediaType::NONE;
};

class UserAgentHeader {
    std::string raw_header;

    public:

    std::string getRawHeader(void)
    {
        return raw_header;
    }

    void setRawHeader(std::string new_raw_header)
    {
        raw_header = new_raw_header;
    }
};

class IfModifiedSinceHeader {
    std::string raw_header;

    public:

    std::string getRawHeader(void)
    {
        return raw_header;
    }

    void setRawHeader(std::string new_raw_header)
    {
        raw_header = new_raw_header;
    }
};

class ConnectionHeader {
    std::string raw_header;

    public:

    std::string getRawHeader(void)
    {
        return raw_header;
    }

    void setRawHeader(std::string new_raw_header)
    {
        raw_header = new_raw_header;
    }
};

class AuthorizationHeader {
    std::string raw_header;

    public:

    std::string getRawHeader(void)
    {
        return raw_header;
    }

    void setRawHeader(std::string new_raw_header)
    {
        raw_header = new_raw_header;
    }
};

enum ProcessingStage {
    METHOD,
    TARGET,
    VERSION,
    HOST,
    HEADERS
};


struct Headers {
    AcceptHeader accept = AcceptHeader();
    UserAgentHeader user_agent = UserAgentHeader();
    IfModifiedSinceHeader modified = IfModifiedSinceHeader();
    ConnectionHeader connection = ConnectionHeader();
    AuthorizationHeader authorization = AuthorizationHeader();
};

struct HTTPRequest
{
    std::string raw_request;
    int index = 0;
    int stringLen = 0;
    ProcessingStage requestStage = ProcessingStage::METHOD;

    MethodType method = MethodType::NO_METHOD;
    RequestTarget target = RequestTarget();
    Version version = Version::NO_VERSION;
    VirtualHost host = VirtualHost();
    Headers headers;
    std::string body; //right now just holds the rest of the message

};

void string_to_request(const std::string& request_string, HTTPRequest* new_request);