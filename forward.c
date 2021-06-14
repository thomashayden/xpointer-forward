// Standard stuff
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// XCB
#include <xcb/xcb.h>
#include <xcb/xproto.h>

// Networking
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

// Look into uinput: https://www.kernel.org/doc/html/v4.12/input/uinput.html


/**
 * Get IP address information given either a hostname or an IP.
 *
 * @param addr hostname or IP address
 * @param port port
 * @param res addrinfo pointer to store results in
 * @param server 1 if running as a server, 0 if running as a client
 * @return 0 if success, non-zero failure
 */
int getIP(const char *addr, const char *port, struct addrinfo **res, int server)
{
    struct addrinfo hints;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (server) {
        hints.ai_flags = AI_PASSIVE;
    }

    if ((status = getaddrinfo(addr, port, &hints, res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    //freeaddrinfo(res);

    return 0;
}

int main(int argc, char* argv[]) {
    int server = -1;
    const char* hostname;
    const char* port = "3490"; // Set to a constant. Might make an argument later

    int option;
    while((option = getopt(argc, argv, "sc")) != -1) {
        switch(option) {
            case 's':
                server = 1;
                break;
            case 'c':
                server = 0;
                break;
        }
    }

    // Server or client must has been specified
    if (server == -1) {
        fprintf(stderr, "-s or -c not specified. Exiting.");
        return -1;
    }

    // Expect exactly 1 additional argument if in client mode which is hostname
    if (!server && argc - optind != 1) {
        fprintf(stderr, "Expected hostname when running as client.");
        return -1;
    }

    if (!server) {
        hostname = argv[optind];
    } else {
        hostname = NULL;
    }



    ///
    /// Xserver setup
    ///

    // Make a connection to the xserver
    xcb_connection_t* conn = xcb_connect(NULL, NULL);

    // Check if any error occured with the connection
    if (xcb_connection_has_error(conn)) {
      fprintf(stderr, "Cannot open display\n");
      exit(EXIT_FAILURE);
    }

    // Get the screen
    xcb_screen_t* s = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_window_t window = s->root;

    xcb_warp_pointer(conn, XCB_NONE, window, 0, 0, 0, 0, 100, 100);

    ///
    /// Network setup
    ///

    // Get IP information
    struct addrinfo *res;
    if ((getIP(hostname, port, &res, server)) != 0) {
        return -1;
    }

    // Create socket
    int sockfd;
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
    {
        fprintf(stderr, "Error creating socket");
        return -1;
    }


    if (server) {
        bind(sockfd, res->ai_addr, res->ai_addrlen);
        listen(sockfd, 5);

        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

        // Close listening port. Only ever accept one connection.
        close(sockfd);

        // Loop until connection is closed, or program terminated
        while (1) {
            // Message is guaranteed to be no more than 8192 bytes
            char buffer[8192] = {0};
            char single[1] = {0};
            // Receive characters one by one and append to buffer until \n is reached
            while (single[0] != '\n') {
                if (recv(new_fd, single, 1, 0) == -1) {
                    fprintf(stderr, "Server closed connection during read");
                    return -1;
                }
                int buffer_len = strlen(buffer);
                buffer[buffer_len] = single[0];
                buffer[buffer_len+1] = '\0';
            }
            fprintf(stdout, buffer);
        }
    }


    if (!server) {
        // Make connection
        if ((connect(sockfd, res->ai_addr, res->ai_addrlen)) != 0) {
            fprintf(stderr, "Error making connection");
            return -1;
        }

        int16_t x_pos = 0;
        int16_t y_pos = 0;
        uint16_t mask = 0;
        xcb_generic_error_t** e;

        while(1) {
            xcb_query_pointer_cookie_t pnt = xcb_query_pointer(conn, window);
            xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, pnt, e);
            if (x_pos != reply->root_x || y_pos != reply->root_y || mask != reply->mask) {
                x_pos = reply->root_x;
                y_pos = reply->root_y;
                mask = reply->mask;
                //fprintf(stdout, "Pointer at (%i, %i)with mask %d\n", x_pos, y_pos, mask);
                /*if (mask & 256) {
                    fprintf(stdout, "Left click\n");
                }
                if (mask & 1024) {
                    fprintf(stdout, "Right click\n");
                }
                if (mask) {
                    fprintf(stdout, "Unknown mask %i\n", mask);
                }*/

                // Create and send message
                char msg[32];
                sprintf(msg, "%d,%d.%d\n", x_pos, y_pos, mask);

                int send_code;
                if ((send_code = send(sockfd, msg, strlen(msg), 0)) == -1) {
                    fprintf(stderr, "Error sending initial message");
                }
            }
        }
    }

    // Free IP address info which is no unneeded
    freeaddrinfo(res);







    int16_t x_pos = 0;
    int16_t y_pos = 0;
    uint16_t mask = 0;
    xcb_generic_error_t** e;

    while (1) {
        xcb_query_pointer_cookie_t pnt = xcb_query_pointer(conn, window);
        xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(conn, pnt, e);
        if (x_pos != reply->root_x || y_pos != reply->root_y || mask != reply->mask) {
            x_pos = reply->root_x;
            y_pos = reply->root_y;
            mask = reply->mask;
            //fprintf(stdout, "Pointer at (%i, %i)with mask %d\n", x_pos, y_pos, mask);
            if (mask & 256) {
                fprintf(stdout, "Left click\n");
            }
            if (mask & 1024) {
                fprintf(stdout, "Right click\n");
            }
            if (mask) {
                fprintf(stdout, "Unknown mask %i\n", mask);
            }
        }
    }

    return 0;
}
