#include <iostream>
#include <curl/curl.h>

int main() {
    // Initialize libcurl
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if (curl) {
        // Set the URL you want to send the request to
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080");  // Replace with your target URL

        // Set the HTTP method (POST in this case)
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // Set the request data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "Hello, World!");

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
