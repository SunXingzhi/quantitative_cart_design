#ifndef DATA_PARSER_H
#define DATA_PARSER_H

#include <stdbool.h>
#include <stdio.h>
typedef enum { QUERY_NONE, QUERY_POINTS, QUERY_NSTART, QUERY_PGET, QUERY_INVALID } query_type_t;

query_type_t parse_client_command(const char *cmd, float out_points[], size_t *out_num_points);
bool parse_mcu_response(const char *resp, float *latitude, float *longitude);

#endif // DATA_PARSER_H
