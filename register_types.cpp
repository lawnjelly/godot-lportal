/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "lroom_manager.h"


void register_lportal_types() {

        ClassDB::register_class<LRoomManager>();
}

void unregister_lportal_types() {
   //nothing to do here
}
