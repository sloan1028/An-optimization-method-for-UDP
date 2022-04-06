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
#include "addressbook.pb.h"

#define MAXDATASIZE 1024
using namespace std;
void* buffer[1024];

// Iterates though all people in the AddressBook and prints info about them.
void ListPeople(const tutorial::Gamer &gamers) {
    for (int i = 0; i < gamers.people_size(); i++) {
        const tutorial::Person_State &person = gamers.people(i);

        cout << "Person ID: " << person.player_id() << endl;
        cout << "  Name: " << person.player_name() << endl;
        if (person.has_current_weapon()) {
            cout << "  current weapon: " << person.current_weapon() << endl;
        }

        for (int j = 0; j < person.weapons_size(); j++) {
            const tutorial::Person_State::Weapon &weapon = person.weapons(j);
            cout << weapon.weapon_name() << ' ';
            cout << weapon.bullet() << endl;
        }
    }
}

// Main function:  Reads the entire address book from a file and prints all
//   the information inside.
int main(int argc, char *argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if(argc < 3) {
        printf("usage: %s ip port\n",argv[0]);
        exit(1);
    }

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serveraddr.sin_addr.s_addr);
    bzero(&(serveraddr.sin_zero),8);

    if((connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr))) == -1) {
        perror("connect");
        exit(1);
    }


    int size = 0, receive_size = 0;
    void* receive_buffer[1024];
    receive_size = read(sockfd, receive_buffer, 1024); 
    //write(1, buffer, size); 
    cout << "receive_size : " << receive_size << endl;

    //LZ4解压
    int max_src_ex_size = 1024;
    int src_ex_size = LZ4_decompress_safe((char*)receive_buffer, (char*)buffer, receive_size, max_src_ex_size);

    string str;
    str.assign((char*)buffer, src_ex_size);
    cout << src_ex_size << endl;
    
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

