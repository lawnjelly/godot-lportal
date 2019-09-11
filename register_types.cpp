/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "lroom.h"
#include "lroom_manager.h"
#include "lportal.h"


void register_lportal_types() {

        ClassDB::register_class<LRoom>();
        ClassDB::register_class<LRoomManager>();
        ClassDB::register_class<LPortal>();
}

void unregister_lportal_types() {
   //nothing to do here
}
