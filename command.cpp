#include "command.h"

Command::Command(char* name): input{ nullptr }, output{ nullptr } {
    this->name = name;
};