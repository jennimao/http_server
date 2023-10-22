#include <iostream>
#include <curl/curl.h>

int main() {
    // Initialize libcurl
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if (curl) {
        // Set the URL you want to send the request to
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/index.html");  // Replace with your target URL

        // Set the Accept header
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: text/html, application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set other headers
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MyUserAgent/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
        curl_easy_setopt(curl, CURLOPT_TIMEVALUE, 1679185951); // Convert the date to UNIX timestamp
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

        // Set the Connection header to "close"
        headers = curl_slist_append(headers, "Connection: close");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the Authorization header
        headers = curl_slist_append(headers, "Authorization: Bearer YourAccessToken");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


        // Send the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up and free resources
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Curl initialization failed." << std::endl;
    }

    return 0;
}
