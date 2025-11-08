#include "HttpClient.h"
#include "secrets.h"

namespace Whisper {
    HttpClient http;

    http_header_t headers[] = {
        //  { "Content-Type", "application/json" },
        //  { "Accept" , "application/json" },
        { "Accept" , "*/*"},
        { NULL, NULL } // NOTE: Always terminate headers will NULL
    };

    typedef struct {
        String name;
        int(*func_ptr)(String);
    } function_t;

    static function_t functions[50];
    static int funcIdx = 0;

    http_request_t request;
    http_response_t response;

    void updateVariable(String name, int* var, int value) {
        *var = value;
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/variableChanged/" + name + "?type=int";
        request.body = String(value);

        http.post(request, response, headers);
    }

    void updateVariable(String name, double* var, double value) {
        *var = value;
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/variableChanged/" + name + "?type=float";
        request.body = String(value);

        http.post(request, response, headers);
    }

    void updateVariable(String name, float* var, float value) {
        *var = value;
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/variableChanged/" + name + "?type=float";
        request.body = String(value);

        http.post(request, response, headers);
    }

    void updateVariable(String name, bool* var, bool value) {
        *var = value;
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/variableChanged/" + name + "?type=bool";
        request.body = String(value);

        http.post(request, response, headers);
    }

    void updateVariable(String name, String* var, String value) {
        *var = value;
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/variableChanged/" + name + "?type=string";
        request.body = String(value);

        http.post(request, response, headers);
    }

    void registerFunction(String name, int(*func_ptr)(String)) {
        functions[funcIdx] = {
            name,
            func_ptr
        };
        funcIdx++;
    }

    void handleCloudFunctions() {
        request.hostname = API_HOST;
        request.port = 80;
        request.path = "/api/popFunc";
        request.body = "";
        http.get(request, response, headers);
        Serial.println(response.body);

        if(response.body == "null") return;

        const char* body = request.body.c_str();
        const char* endbody = strchr(body, '/');

        int len = strlen(body);
        int endlen = strlen(endbody);

        String name = request.body.substring(0, len - endlen);
        String arg = request.body.substring(endbody - body);
        Serial.println(name);

        for(int i = 0; i < funcIdx; i++) {
            Serial.println((unsigned int)functions[i].func_ptr);
            //if(name == functions[i].name) functions[i].func_ptr(arg);
        }
    }
}