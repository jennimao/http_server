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
    std::string getStr(void) {
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
    void parse(std::string newHost)
    {
        setTarget(newHost);
    }
    std::string getStr(void)
    {
        return virtualHost;
    }
};

enum MediaType{
    TEXT,
    HTML,
    IMAGE,
    PNG,
    JPEG,
    NONE
};

static std::string acceptedTypesStrings[] = {"text", "html", "image", "png", "jpeg"};

class AcceptHeader {
    std::string raw_header;
    std::array<std::tuple<MediaType, MediaType>, 2> accepted_types;

    public:

    void parse(std::string newHeader)
    {
        //saving raw val
        raw_header = newHeader;

        /* //parsing
        size_t index = 0;
        size_t value_len = newHeader.length();

        while(index < value_len)
        {
            newHeader.substr(index, value_len).find(',');
            if()
        } */

    }
    
    std::string getStr(void)
    {
        return raw_header;
    }
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

    void parse (std::string new_raw_header)
    {
        setRawHeader(new_raw_header);
    }
     
    std::string getStr(void)
    {
        return raw_header;
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

    void parse (std::string new_raw_header)
    {
        setRawHeader(new_raw_header);
    }

    std::string getStr(void)
    {
        return raw_header;
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

    void parse (std::string new_raw_header)
    {
        setRawHeader(new_raw_header);
    }

    std::string getStr(void)
    {
        return raw_header;
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

    void parse (std::string new_raw_header)
    {
        setRawHeader(new_raw_header);
    }

    std::string getStr(void)
    {
        return raw_header;
    }
};

enum ProcessingStage {
    METHOD,
    TARGET,
    VERSION,
    HOST,
    HEADERS,
    BODY
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

int string_to_request(const std::string& request_string, HTTPRequest* new_request);