#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <google/protobuf/util/json_util.h>
#include <lz4.h>
#include "rudp.h"
#include "addressbook.pb.h"
#include "rudp.c"
using google::protobuf::util::BinaryToJsonStream;

int sockfd;
char buffer[1024];
char data[512];
char *recv_buff[1024];
struct sockaddr_in client_sockaddr;
#define BACKLOG 10
#define MAXDATASIZE 5

using namespace std;

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("server close\n");
        close(sockfd);
        exit(1);
    }
}

void out_addr(struct sockaddr_in *clientaddr)
{
    int port = ntohs(clientaddr->sin_port);
    char ip[16];
    memset(ip, 0, sizeof(ip));
    inet_ntop(AF_INET, &clientaddr->sin_addr.s_addr, ip, sizeof(ip));
    printf("client: %s(%d) connected\n", ip, port);
}

void server_sock_init(struct sockaddr_in &sockaddr, char *port)
{
    sockaddr.sin_family = AF_INET; // IPv4
    sockaddr.sin_port = htons(atoi(port));
    sockaddr.sin_addr.s_addr = INADDR_ANY;
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

int rudp_output(int sockfd, struct sockaddr_in &sockaddr)
{
    int len = sizeof(sockaddr);
    int recv_size = recvfrom(sockfd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&sockaddr, (socklen_t *)&len);
    cout << "recv_size : " << recv_size << endl;
    return recv_size;
}

// 打印并发送已经封装的包
static void rudp_send(struct rudp_package *p, int sockfd)
{
    static int idx = 0;
    int size = 0;
    printf("send id : %d ", idx++);
    while (p)
    {
        // if (rand() % 5 == 0) break;
        memcpy(data, p->buffer, p->sz);
        int i;
        for (i = 0; i < p->sz; i++)
        {
            printf("%02x ", (uint8_t)data[i]);
        }
        size += p->sz + 1;
        socklen_t len = sizeof(client_sockaddr);
        int send_num;
        if (p->sz > 0)
            printf("发送了发送了 发送了%d的数量\n", p -> sz);
            send_num = sendto(sockfd, (void *)data, p->sz, 0, (struct sockaddr *)&client_sockaddr, len);
        if (send_num < 0)
        {
            perror("sendto error:");
            exit(1);
        }
        p = p->next;
    }
    printf("\n");
}

// A在B上改变了什么
tutorial::Gamer compare_state(tutorial::Gamer &A, tutorial::Gamer &B)
{
    tutorial::Gamer res;
    auto &people = res.people();
    for (int i = 0; i < A.people_size(); i++)
    {
        const tutorial::Person_State &a = A.people(i);
        bool found = false, has_changed = false;
        tutorial::Person_State t;
        for (int j = 0; j < B.people_size(); j++)
        {
            const tutorial::Person_State &b = B.people(j);
            if (a.player_id() == b.player_id() && a.player_name() == b.player_name())
            {
                found = true;
                t.set_player_id(a.player_id());
                t.set_player_name(a.player_name());
                // t->set_player_id(a.player_id());
                // t->set_player_name(a.player_name());
                if (a.player_hp() != b.player_hp())
                {
                    t.set_player_hp(a.player_hp());
                    // t->set_player_hp(a.player_hp());
                    has_changed = true;
                }

                if (a.player_armor() != b.player_armor())
                {
                    t.set_player_armor(a.player_armor());
                    // t->set_player_armor(a.player_armor());
                    has_changed = true;
                }

                if (a.current_weapon() != b.current_weapon())
                {
                    t.set_current_weapon(a.current_weapon());
                    // t->set_current_weapon(a.current_weapon());
                    has_changed = true;
                }

                for (int k = 0; k < 3; k++)
                {
                    if (a.player_position(k) != b.player_position(k))
                    {
                        t.set_player_position(k, a.player_position(k));
                        // t->set_player_position(k, a.player_position(k));
                        has_changed = true;
                    }
                }

                //武器修改
                auto w1 = a.weapons();
                auto w2 = b.weapons();

                for (int k = 0; k < w1.size(); k++)
                {
                    bool find = false, changed = false;
                    auto new_weapon = t.add_weapons();
                    for (int l = 0; l < w2.size(); l++)
                    {
                        if (w1[k].weapon_name() == w2[l].weapon_name())
                        {
                            find = true;
                            if (w1[k].bullet() != w2[l].bullet())
                            {
                                changed = true;
                            }
                            break;
                        }
                    }
                    if (!find || changed)
                    {
                        new_weapon->set_weapon_name(w1[k].weapon_name());
                        new_weapon->set_bullet(w1[k].bullet());
                        has_changed = true;
                    }
                    else
                    {
                        new_weapon->Clear();
                    }
                }

                if (has_changed)
                {
                    auto new_people = res.add_people();
                    new_people->CopyFrom(t);
                }
                break;
            }
        }
        if (!found)
        { // 如果没找到说明是新加的
            auto new_people = res.add_people();
            new_people->CopyFrom(a);
        }
    }

    return res;
}

// Main function:  Reads the entire address book from a file,
//   adds one person based on user input, then writes it back out to the same
//   file.
int main(int argc, char *argv[])
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc < 2)
    {
        printf("usage: %s #port\n", argv[0]);
        exit(1);
    }
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("signal sigint error");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    int recvbytes;
    socklen_t sin_size;
    int client_fd;
    char buf[MAXDATASIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Socket");
        exit(1);
    }
    printf("Socket success!,sockfd=%d\n", sockfd);

    server_sock_init(serveraddr, argv[1]);
    int len = sizeof(serveraddr);
    int recv_num;
    int send_num;
    char send_buf[20] = "i am server!";
    char recv_buf[20];

    if ((bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr))) == -1)
    {
        perror("bind");
        exit(-1);
    }

    printf("bind success!\n");
    struct rudp *U = rudp_new(1, 5);

    printf("server wait:\n");
    recv_num = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&client_sockaddr, (socklen_t *)&len);
    if (recv_num < 0)
    {
        perror("recvfrom error:");
        exit(1);
    }
    out_addr(&client_sockaddr);

    recv_buf[recv_num] = '\0';
    printf("server receive %d bytes: %s\n", recv_num, recv_buf);

    tutorial::Gamer person_state;
    tutorial::Gamer pre_person_state;

    int i = 1;
    while (1)
    {
        if (i <= 10)
        {
            string file_path = "data/protobuf" + to_string(i) + ".data";

            {
                // Read the existing address book.
                fstream input(file_path, ios::in | ios::binary);
                if (!input)
                {
                    cout << file_path << ": File not found.  Creating a new file." << endl;
                }
                else if (!person_state.ParseFromIstream(&input))
                {
                    cerr << "Failed to parse preson state." << endl;
                    return -1;
                }
            }

            tutorial::Gamer state_difference = compare_state(person_state, pre_person_state);

            int size = state_difference.ByteSizeLong();
            void *buffer = malloc(size);
            state_difference.SerializeToArray(buffer, size);
            cout << "压缩前的数据: " << size << endl;
            /*
            //打印数据
            string str3;
            str3.assign((char *)buffer, size);
            cout << str3 << endl;
            */

            // LZ4压缩
            int src_size = size + 1;
            int max_dst_size = LZ4_compressBound(src_size);
            char *dst = new char[max_dst_size];
            int dst_size = LZ4_compress_default((char *)buffer, dst, src_size, max_dst_size);

            /*
            send_num = sendto(sockfd, dst, dst_size, 0, (struct sockaddr *)&client_sockaddr, len);
            if (send_num < 0)
            {
                perror("sendto error:");
                exit(1);
            }
            */

            cout << "压缩后带发送的数据 :" << dst_size << endl;
            rudp_input(U, dst, dst_size);

            delete[] dst;
            dst = NULL;
        }

        rudp_send(rudp_update(U, NULL, 0, 1), sockfd); // 发送1

        pre_person_state = person_state;

        
        int recv_size = rudp_output(sockfd, client_sockaddr); // 接收1
        if(recv_size) rudp_update(U, recv_buff, recv_size, 0);
        
        sleep(1);
        i++;
    }

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
