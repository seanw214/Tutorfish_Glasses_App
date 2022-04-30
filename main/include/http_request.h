#ifndef HTTP_REQUEST_H__
#define HTTP_REQUEST_H__

size_t http_get_request(char *hostname, char *path, char *query, char *session_cookie);
size_t http_post_request(char *hostname, char *path, char *post_data, char *session_cookie);

#endif //HTTP_REQUEST_H__