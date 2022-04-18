#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <lz4.h>
#include "rudp.h"
#include "rudp.c"
#include "addressbook.pb.h"

#define MAXDATASIZE 1024
using namespace std;
void *buffer[1024];
char data[512];
struct sockaddr_in server_addr;
/*
int rudp_connect(struct rudp *U, int sockfd, struct sockaddr_in &sockaddr)
{
    char send_connect[10] = "connect";
    rudp_send(U, send_connect, strlen(send_connect));
    dump(rudp_update(U, NULL, 0, 1), sockfd);

    socklen_t len = sizeof(server_addr);
    int recv_size = 0;
    char *recv_buff[50];
    recv_size = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&server_addr, (socklen_t *)&len);
    dump(rudp_update(U, recv_buff, recv_size, 1), sockfd);
    recv_size = dump_recv(U, recv_buff);
    cout << "recv_buff" << recv_buff << endl;
}
*/

// Iterates though all people in the AddressBook and prints info about them.
void ListPeople(const tutorial::Gamer &gamers)
{
    for (int i = 0; i < gamers.people_size(); i++)
    {
        const tutorial::Person_State &person = gamers.people(i);

        cout << "Person ID: " << person.player_id() << endl;
        cout << "  Name: " << person.player_name() << endl;
        if (person.has_current_weapon())
        {
            cout << "  current weapon: " << person.current_weapon() << endl;
        }

        for (int j = 0; j < person.weapons_size(); j++)
        {
            const tutorial::Person_State::Weapon &weapon = person.weapons(j);
            cout << weapon.weapon_name() << ' ';
            cout << weapon.bullet() << endl;
        }
    }
}

void client_sock_init(struct sockaddr_in &sockaddr, char *ip, char *port)
{
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(atoi(port));
    inet_pton(AF_INET, ip, &sockaddr.sin_addr.s_addr);
    bzero(&(sockaddr.sin_zero), 8);
}

static int dump_recv(struct rudp *U, char **buff) // 这里的dump没有考虑多包的情况
{
    char tmp[MAX_PACKAGE];
    int n, size = 0;
    while ((n = rudp_recv(U, tmp)))
    {
        if (n < 0)
        {
            printf("CORRPUT\n");
            break;
        }
        int i;
        printf("RECV ");
        for (i = 0; i < n; i++)
        {
            printf("%02x ", (uint8_t)tmp[i]);
        }
        printf("\n");
        memcpy(buff, tmp, n);
        size = n;
    }
    return size;
}

static void dump(struct rudp_package *p, int sockfd)
{
    static int idx = 0;
    int size = 0;
    printf("%d : ", idx++);
    while (p)
    {
        memcpy(data, p->buffer, p->sz);
        int i;
        for (i = 0; i < p->sz; i++)
        {
            printf("%02x ", (uint8_t)data[i]);
        }
        size += p->sz + 1;
        socklen_t len = sizeof(server_addr);
        int send_num;
        if (p->sz > 0)
            send_num = sendto(sockfd, (void *)data, p->sz, 0, (struct sockaddr *)&server_addr, len);
        if (send_num < 0)
        {
            perror("sendto error:");
            exit(1);
        }
        p = p->next;
    }
    printf("\n");
}

// Main function:  Reads the entire address book from a file and prints all
//   the information inside.
int main(int argc, char *argv[])
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc < 3)
    {
        printf("usage: %s ip port\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(1);
    }

    client_sock_init(server_addr, argv[1], argv[2]);
    int len = sizeof(server_addr);

    int send_num;
    char send_buf[20] = "hey, who are you?";

    printf("client send: %s\n", send_buf);

    send_num = sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr *)&server_addr, len);

    if (send_num < 0)
    {
        perror("sendto error:");
        exit(1);
    }

    struct rudp *U = rudp_new(1, 5);
    /*
    int con = rudp_connect(U, sockfd, server_addr);
    if (con < 0)
    {
        printf("connection failed\n");
        exit(1);
    }
    else
    {
        printf("connect success!\n");
    }
    */
    for (int i = 1; i <= 5; i++)
    {

        int size = 0, recv_size = 0;
        char *recv_buff[1024];
        recv_size = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&server_addr, (socklen_t *)&len);
        dump(rudp_update(U, recv_buff, recv_size, 1), sockfd);
        recv_size = dump_recv(U, recv_buff);

        // write(1, buffer, size);
        cout << "recv_size : " << recv_size << endl;

        // LZ4解压
        int max_src_ex_size = 1024;
        int src_ex_size = 0;
        if (recv_size != 0)
            src_ex_size = LZ4_decompress_safe((char *)recv_buff, (char *)buffer, recv_size, max_src_ex_size);
        string str;
        str.assign((char *)buffer, src_ex_size);
        cout << "Decompression size : " << src_ex_size << endl;

        tutorial::Gamer person_state;
        person_state.ParseFromArray(buffer, src_ex_size);

        /*
        {
            // Read the existing address book.
            fstream input(argv[1], ios::in | ios::binary);
            if (!person_state.ParseFromIstream(&input)) {
                cerr << "Failed to parse person state." << endl;
                return -1;
            }
        }
        */

        ListPeople(person_state);
        sleep(1);
    }

    close(sockfd);

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
