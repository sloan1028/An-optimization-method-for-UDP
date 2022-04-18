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
#include "rudp.h"
#include "addressbook.pb.h"
#include "rudp.c"
using google::protobuf::util::BinaryToJsonStream;

using namespace std;

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

    int hp = 0;
    printf("Player Hp Now: ");
    cin >> hp;
    person->set_player_hp(hp);

    int armor = 0;
    printf("Player Armor Now: ");
    cin >> armor;
    person->set_player_armor(armor);

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

void ModifyPersonState(tutorial::Gamer &person_state)
{
    int id = 0;
    std::cout << "input the player's ID: ";
    std::cin >> id;
    bool success = false;
    for (int i = 0; i < person_state.people_size(); i++)
    {
        tutorial::Person_State *person = person_state.mutable_people(i);
        if (person -> player_id() != id)
            continue;
        success = true;

        cin.get();
        string name;
        cout << "Player name : " << person -> player_name() << "(or enter to jump) : ";
        getline(cin, name);
        if(!name.empty()) person->set_player_name(name);;

        cin.get();
        string c_weapon;
        cout << "Current_Weapon name : " << person -> current_weapon() << " (or enter to jump) : ";
        getline(cin, c_weapon);
        if(!c_weapon.empty()) person->set_current_weapon(c_weapon);

        for (int i = 0; i < 3; i++)
        {
            cin.get();
            printf("Player Position Now (%d) :%lf : (or enter to jump) : ", i, person -> player_position(i));
            string pos;
            getline(cin , pos);
            if(!pos.empty()) person->set_player_position(i, stof(pos));
            
        }

        cin.get();
        string hp;
        cout << "Player hp : " << person -> player_hp() << "(or enter to jump) : ";
        getline(cin, hp);
        if(!hp.empty()) person->set_player_hp(stoi(hp));

        cin.get();
        string armor;
        cout << "Player armor : " << person -> player_armor() << "(or enter to jump) : ";
        getline(cin, armor);
        if(!hp.empty()) person->set_player_armor(stoi(armor));

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

    if (!success)
    {
        std::cout << "the ID not found!" << endl;
    }
    else
    {
        std::cout << "modify success!" << endl;
    }
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
        printf("usage: %s #data_address\n", argv[0]);
        exit(1);
    }

    printf("%s\n", argv[1]);

    while (1)
    {

        tutorial::Gamer person_state;

        {
            // Read the existing address book.
            fstream input(argv[1], ios::in | ios::binary);
            if (!input)
            {
                cout << argv[1] << ": File not found.  Creating a new file." << endl;
            }
            else if (!person_state.ParseFromIstream(&input))
            {
                cerr << "Failed to parse preson state." << endl;
                return -1;
            }
        }

        int op = 0;
        printf("choose your option(1: add person, 2: modify person) : \n");
        cin >> op;
        switch (op)
        {
        case 1:
            // Add an person.
            PromptForPerson(person_state.add_people());
            break;
        case 2:
            ModifyPersonState(person_state);
            break;
        default:
            break;
        }

        int size = person_state.ByteSizeLong();
        void *buffer = malloc(size);
        person_state.SerializeToArray(buffer, size);

        //打印数据
        string str3;
        str3.assign((char *)buffer, size);
        cout << str3 << endl;

        string new_address = argv[1];

        //修改地址, 如果不修改地址注释下面这行
        new_address[13] += 1;

        // protobuf2json
        string json_string;
        if (!proto_to_json(person_state, json_string))
        {
            std::cout << "protobuf convert json failed!" << std::endl;
            return -1;
        }
        string json_path = new_address.substr(0, 14) + ".json";
        cout << json_path << endl;
        write_to_file(json_path.c_str(), json_string);

        std::cout << "protobuf convert json done!" << std::endl
                  << json_string << std::endl;
        std::cout << "json size: " << json_string.size() << endl;
        // protobuf2json

        {
            // Write the new address book back to disk.
            fstream output(new_address.c_str(), ios::out | ios::trunc | ios::binary);
            if (!person_state.SerializeToOstream(&output))
            {
                cerr << "Failed to write person state." << endl;
                return -1;
            }
        }
    }

    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
