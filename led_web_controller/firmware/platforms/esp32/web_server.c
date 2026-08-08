#include "web_server.h"
#include "led_controller.h"
#include "web_page.h"
#include "app_config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WEB";
static httpd_handle_t s_server;

static esp_err_t send_json(httpd_req_t *req, const char *json, const char *status)
{
    httpd_resp_set_type(req, "application/json");
    if (status != NULL) {
        httpd_resp_set_status(req, status);
    }
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, WEB_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    led_state_t state;
    if (led_get_state(&state) != ESP_OK) {
        return send_json(req, "{\"error\":\"state unavailable\"}", "500 Internal Server Error");
    }
    char json[128];
    snprintf(json, sizeof(json), "{\"on\":%s,\"r\":%u,\"g\":%u,\"b\":%u,\"color\":\"%s\"}",
             state.enabled ? "true" : "false", state.red, state.green, state.blue, state.color);
    return send_json(req, json, NULL);
}

static esp_err_t command_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    if (strcmp(uri, "/api/led/on") == 0) {
        ESP_LOGI(TAG, "Client requested LED ON");
        return led_on() == ESP_OK ? send_json(req, "{\"ok\":true}", NULL) : send_json(req, "{\"error\":\"LED failure\"}", "500 Internal Server Error");
    }
    if (strcmp(uri, "/api/led/off") == 0) {
        ESP_LOGI(TAG, "Client requested LED OFF");
        return led_off() == ESP_OK ? send_json(req, "{\"ok\":true}", NULL) : send_json(req, "{\"error\":\"LED failure\"}", "500 Internal Server Error");
    }

    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len == 0 || query_len >= HTTP_MAX_URI_LEN) {
        return send_json(req, "{\"error\":\"query required\"}", "400 Bad Request");
    }
    char query[HTTP_MAX_URI_LEN];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        return send_json(req, "{\"error\":\"invalid query\"}", "400 Bad Request");
    }
    char name[16];
    if (httpd_query_key_value(query, "name", name, sizeof(name)) == ESP_OK) {
        ESP_LOGI(TAG, "Client requested color: %s", name);
        return led_set_color(name) == ESP_OK ? send_json(req, "{\"ok\":true}", NULL) : send_json(req, "{\"error\":\"unknown color\"}", "400 Bad Request");
    }
    char value[8];
    int rgb[3];
    const char *keys[] = {"r", "g", "b"};
    for (int i = 0; i < 3; ++i) {
        if (httpd_query_key_value(query, keys[i], value, sizeof(value)) != ESP_OK) {
            return send_json(req, "{\"error\":\"r, g and b are required\"}", "400 Bad Request");
        }
        char *end = NULL;
        long parsed = strtol(value, &end, 10);
        if (value[0] == '\0' || *end != '\0' || parsed < 0 || parsed > 255) {
            return send_json(req, "{\"error\":\"RGB values must be 0..255\"}", "400 Bad Request");
        }
        rgb[i] = (int)parsed;
    }
    ESP_LOGI(TAG, "Client requested color: RGB(%d,%d,%d)", rgb[0], rgb[1], rgb[2]);
    return led_set_rgb((uint8_t)rgb[0], (uint8_t)rgb[1], (uint8_t)rgb[2]) == ESP_OK ? send_json(req, "{\"ok\":true}", NULL) : send_json(req, "{\"error\":\"LED failure\"}", "500 Internal Server Error");
}

esp_err_t web_server_start(void)
{
    if (s_server != NULL) return ESP_OK;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.max_uri_handlers = 8;
    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) return err;
    static const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_handler};
    static const httpd_uri_t status = {.uri = "/api/status", .method = HTTP_GET, .handler = status_handler};
    static const httpd_uri_t on = {.uri = "/api/led/on", .method = HTTP_GET, .handler = command_handler};
    static const httpd_uri_t off = {.uri = "/api/led/off", .method = HTTP_GET, .handler = command_handler};
    static const httpd_uri_t color = {.uri = "/api/led/color", .method = HTTP_GET, .handler = command_handler};
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &status));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &on));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &off));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &color));
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
