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

void sockaddr_init(struct sockaddr_in &serveraddr, char* ip, char* port){
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(port));
    inet_pton(AF_INET, ip, &serveraddr.sin_addr.s_addr);
    bzero(&(serveraddr.sin_zero), 8);
}

static void
dump_recv(struct rudp *U)
{
    char tmp[MAX_PACKAGE];
    int n;
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
    }
}

static void dump(struct rudp_package *p)
{
    static int idx = 0;
    printf("%d : ", idx++);
    while (p)
    {
        printf("(");
        int i;
        for (i = 0; i < p->sz; i++)
        {
            printf("%02x ", (uint8_t)p->buffer[i]);
        }
        printf(") ");
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

    struct sockaddr_in serveraddr;
    sockaddr_init(serveraddr, argv[1], argv[2]);
    int len = sizeof(serveraddr);

    int send_num;
    int recv_num;
    char send_buf[20] = "hey, who are you?";

    printf("client send: %s\n", send_buf);

    send_num = sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr *)&serveraddr, len);

    if (send_num < 0)
    {
        perror("sendto error:");
        exit(1);
    }
    
    struct rudp *U = rudp_new(1, 5);
    int size = 0, recv_size = 0;
    char *recv_buff[1024];
    recv_size = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&serveraddr, (socklen_t *)&len);
    cout << recv_size << ' ' << endl;
    dump(rudp_update(U, recv_buff, recv_size, 1));
    // dump_recv(U);

    char tmp[MAX_PACKAGE];
    int n, tmp_size = 0;
    while ((n = rudp_recv(U, tmp)))
    {
        if (n < 0)
        {
            printf("CORRPUT\n");
            break;
        }
        int i;
        printf("RECV ");
        tmp_size = n;
        for (i = 0; i < n; i++)
        {
            printf("%02x ", (uint8_t)tmp[i]);
        }
        printf("\n");
    }
    memcpy(recv_buff, tmp, tmp_size);
    recv_size = tmp_size;

    // write(1, buffer, size);
    cout << "recv_size : " << recv_size << endl;

    // LZ4解压
    int max_src_ex_size = 1024;
    int src_ex_size = LZ4_decompress_safe((char *)recv_buff, (char *)buffer, recv_size, max_src_ex_size);

    string str;
    str.assign((char *)buffer, src_ex_size);
    cout << "Decompression size : " << src_ex_size << endl;

    close(sockfd);

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

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
