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

bool proto_to_json(const google::protobuf::Message &message, std::string &json)
{
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    return MessageToJsonString(message, &json, options).ok();
}

void write_to_file(const char *filename, string str)
{
    std::ofstream of(filename, ios::trunc);
    if (of)
    {
        of.write(str.c_str(), str.size());
        of.close();
    }
}

// This function fills in a Person message based on user input.
void PromptForPerson(tutorial::Person_State *person)
{
    cout << "Enter person ID number: ";
    int id;
    cin >> id;
    person->set_player_id(id);
    cin.ignore(256, '\n');

    cout << "Player name: ";
    getline(cin, *person->mutable_player_name());

    cout << "Current_Weapon name: ";
    getline(cin, *person->mutable_current_weapon());

    for (int i = 0; i < 3; i++)
    {
        printf("Player Position Now (%d):", i);
        float p;
        cin >> p;
        person->add_player_position(p);
    }

    while (true)
    {
        cin.get();
        cout << "Enter a weapon (or leave blank to finish): ";
        string weapon_name;
        getline(cin, weapon_name);
        if (weapon_name.empty())
        {
            break;
        }
        cout << "bullet numsber: ";
        int bullet;
        cin >> bullet;

        tutorial::Person_State::Weapon *weapon = person->add_weapons();
        weapon->set_weapon_name(weapon_name);
        weapon->set_bullet(bullet);
    }
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

static void dump(struct rudp_package *p, int sockfd)
{
    static int idx = 0;
    int size = 0;
    printf("%d : ", idx++);
    while (p)
    {
        /*
        string str;
        str.assign(p->buffer, p->sz);
        cout << "|||" << str << "|||" << endl;
        */
        memcpy(data, p->buffer, p->sz);
        int i;
        for (i = 0; i < p->sz; i++)
        {
            printf("%02x ", (uint8_t)data[i]);
        }
        size += p->sz + 1;
        socklen_t len = sizeof(client_sockaddr);
        cout << "hello" << endl;
        int send_num = sendto(sockfd, (void *)data, p->sz, 0, (struct sockaddr *)&client_sockaddr, len);
        if (send_num < 0)
        {
            perror("sendto error:");
            exit(1);
        }
        p = p->next;
    }
    printf("\n");
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

    struct sockaddr_in server_sockaddr;
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

    server_sockaddr.sin_family = AF_INET; // IPv4
    server_sockaddr.sin_port = htons(atoi(argv[1]));
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_sockaddr.sin_zero), 8);
    int len = sizeof(server_sockaddr);
    int recv_num;
    int send_num;
    char send_buf[20] = "i am server!";
    char recv_buf[20];

    if ((bind(sockfd, (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr))) == -1)
    {
        perror("bind");
        exit(-1);
    }

    printf("bind success!\n");
    struct rudp *U = rudp_new(1, 5);

    while (1)
    {
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

        {
            // Read the existing address book.
            fstream input(argv[2], ios::in | ios::binary);
            if (!input)
            {
                cout << argv[2] << ": File not found.  Creating a new file." << endl;
            }
            else if (!person_state.ParseFromIstream(&input))
            {
                cerr << "Failed to parse preson state." << endl;
                return -1;
            }
        }

        // Add an person.
        // PromptForPerson(person_state.add_person());

        char data[1024];

        int size = person_state.ByteSizeLong();
        void *buffer = malloc(size);
        person_state.SerializeToArray(buffer, size);
        /*
        //打印数据
        string str3;
        str3.assign((char *)buffer, size);
        cout << str3 << endl;
        */
        // protobuf2json
        /*
        string json_string;
        if(!proto_to_json(person_state, json_string)) {
            std::cout << "protobuf convert json failed!" << std::endl;
            return -1;
        }

        write_to_file("data.json", json_string);


        std::cout << "protobuf convert json done!" << std::endl
                << json_string << std::endl;
        std::cout << "json size: " << json_string.size() << endl;
        */

        // protobuf2json

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
        char t1[] = "abcd";
        char t2[] = "efgh";
        cout << "dst_size :" << dst_size << endl;
        rudp_send(U, dst, dst_size);

        dump(rudp_update(U, NULL, 0, 1), sockfd);

        delete[] dst;
        dst = NULL;

        /*
        {
        // Write the new address book back to disk.
        fstream output(argv[1], ios::out | ios::trunc | ios::binary);
        if (!person_state.SerializeToOstream(&output)) {
            cerr << "Failed to write person state." << endl;
            return -1;
        }
        }
        */
    }

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
