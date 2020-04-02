// defines

// extra verbose print_lines
//#define LDEBUG_VERBOSE

// frame debug strings
#define LDEBUG_CAMERA
#define LDEBUG_LIGHTS
#define LDEBUG_LIGHT_AFFECTED_ROOMS
//#define LDEBUG_DOB_VISIBILITY

#define LPORTAL_DOBS_NO_SOFTSHOW
//#define LPORTAL_DOBS_AUTO_UPDATE

//#define LDEBUG_UNMERGE

// single compilation unit
#include "register_types.cpp"
#include "ldebug.cpp"
#include "ldoblist.cpp"
#include "lroom.cpp"
#include "lroom_manager.cpp"
#include "lroom_converter.cpp"
#include "lportal.cpp"
#include "lplanes_pool.cpp"
#include "ldob.cpp"
#include "lbound.cpp"
#include "lbitfield_dynamic.cpp"
#include "lhelper.cpp"
#include "lscene_saver.cpp"
#include "ltrace.cpp"
#include "lmain_camera.cpp"
#include "larea.cpp"
#include "ldae_exporter.cpp"

