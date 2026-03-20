#include "data_parser.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

query_type_t parse_client_command(const char *cmd, float out_points[], size_t *out_num_points)
{
        if (!cmd || cmd[0] != '@')
        {
                return QUERY_INVALID;
        }

        size_t len = strlen(cmd);
        if (len < 3 || cmd[len - 1] != '*')
        {
                return QUERY_INVALID;
        }

        char c = cmd[1];
        switch (c){
                case 'p':
                {
                        // @p/....*
                        if (out_points && out_num_points)
                        {
                                char tmp[256];
                                strncpy(tmp, cmd + 3, len - 4);
                                tmp[len - 4] = '\0';
                                size_t count = 0;
                                char *token = strtok(tmp, "/");
                                while (token && count + 1 < MAX_POINTS * 2)
                                {
                                        out_points[count++] = strtof(token, NULL);
                                        token = strtok(NULL, "/");
                                }
                                *out_num_points = count / 2;
                        }
                        return QUERY_POINTS;
                }
                case 'q':
                {
                        if (strncmp(cmd + 2, "Nstart", 6) == 0)
                                return QUERY_NSTART;
                        if (strncmp(cmd + 2, "Pget", 4) == 0)
                                return QUERY_PGET;
                        return QUERY_INVALID;
                }
                default:
                        return QUERY_INVALID;
        }
}

bool parse_mcu_response(const char *resp, float *latitude, float *longitude)
{
        if (!resp || resp[0] != '@')
                return false;
        size_t len = strlen(resp);
        if (len < 3 || resp[len - 1] != '*')
                return false;

        if (resp[1] == 'g')
        {
                float lat = 0, lon = 0;
                char buf[256];
                size_t to_copy = (len - 1 < sizeof(buf) - 1) ? (len - 1) : (sizeof(buf) - 1);
                memcpy(buf, resp, to_copy);
                buf[to_copy] = '\0';
                char *p = strtok(buf, "/"); // @g
                p = strtok(NULL, "/");
                if (!p)
                        return false;
                lat = strtof(p, NULL);
                p = strtok(NULL, "/");
                if (!p)
                        return false;
                lon = strtof(p, NULL);
                if (latitude)
                        *latitude = lat;
                if (longitude)
                        *longitude = lon;
                return true;
        }
        return false;
}
