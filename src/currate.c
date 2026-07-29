#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define VERSION "1.0.1"
#define DATE "2026-04-26"

struct memory {
    char* response;
    size_t size;
};

struct memory get_currency_rate(const char* base, const char* rate);
size_t write_callback(void* data, size_t size, size_t nmemb, void* userp);
double parse_and_calc(double num, char* response);

int main(int argc, char** argv)
{
    double num, result;
    char* base = NULL;
    char* rate = NULL;
    struct memory chunk;

    if (argc < 2) {
        fprintf(stderr, "usage: currate <base> <rate> [num]\n");
        return 2;
    }

    if (strcmp(argv[1], "--help") == 0
    || strcmp(argv[1], "-h") == 0) {
        printf(
            "usage: currate <base> <rate> [num]\n"
            "e.g.:\n"
            "   currate CHF JPY 50\n"
            "\n"
            "currate uses API of currencyrateapi.com\n"
            "You need to connect to internet.\n"
            );
        exit(0);
    } else if (strcmp(argv[1], "--version") == 0
    || strcmp(argv[1], "-v") == 0) {
        printf(
            "currate %s\n"
            "A currency convertor, using API of currencyrateapi.com\n"
            "\n"
            "Written by Yutaka Goy,\n"
            "in C, on %s\n"
            , VERSION, DATE);
        exit(0);
    }

    if (argc < 3) {
        fprintf(stderr, "usage: currate <base> <rate> [num]\n");
        return 2;
    }

    base = argv[1];
    rate = argv[2];

    if (argc > 3)
        num = atoi(argv[3]);
    else
        num = 1;

    chunk = get_currency_rate(base, rate);

    result = parse_and_calc(num, chunk.response);

    if (result == -1) {
        free(chunk.response);
        return 1;
    }

    printf("%f %s = %f %s\n", num, base, result, rate);

    free(chunk.response);
    return 0;
}

struct memory get_currency_rate(const char* base, const char* rate)
{
    CURL* curl;
    CURLcode res;
    char* url;
    struct memory chunk;

    url = malloc(128);
    snprintf(url, 128, "https://currencyrateapi.com/api/latest?base=%s&codes=%s", base, rate);

    chunk.response = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "currate: Failed to get from %s\n", url);
            free(url);
            free(chunk.response);
            exit(1);
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    free(url);

    return chunk;
}

size_t write_callback(void* data, size_t size, size_t nmemb, void* userp)
{
    size_t total = size * nmemb;
    struct memory *mem = (struct memory*) userp;
    char* ptr;

    ptr = realloc(mem->response, mem->size + total + 1);
    if (!ptr)
        return 0;

    mem->response = ptr;
    memcpy(mem->response + mem->size, data, total);
    mem->size += total;
    mem->response[mem->size] = '\0';

    return total;
}

double parse_and_calc(double num, char* response)
{
    double ret;
    struct json_object* root;
    struct json_object* success_obj;
    json_bool success;

    struct json_object* errors_obj;
    struct json_object* error_obj;
    char* error;

    struct json_object* rates_obj;

    root = json_tokener_parse(response);

    if (!json_object_object_get_ex(root, "success", &success_obj)) {
        fprintf(stderr, "currate: No \"success\" key\ndebug: response: %s\n", response);
        json_object_put(root);
        return -1;
    }

    success = json_object_get_boolean(success_obj);

    if (!success) {
        fprintf(stderr,
            "currate: It didn't work.\n"
            "debug: response: %s\n", response);
        return -1;
    }

    if (!json_object_object_get_ex(root, "rates", &rates_obj)) {
        fprintf(stderr, "currate: Failed to read rates, no \"rates\" key.\n"
            "debug: response: %s\n", response);
        json_object_put(root);
        return -1;
    }

    json_object_object_foreach(rates_obj, key, val) {
        ret = json_object_get_double(val);
    }

    json_object_put(root);

    ret *= num;
    return ret;
}
