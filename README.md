# godot-lportal
Portal rendering module for Godot

Work in progress, not yet functional

## Roadmap
* Auto conversion of named room spatials and portal mesh instances to LRoom and LPortal DONE
* Auto creation of mirror portals DONE
* Recursive determine visibility PARTIAL
  * (basic function works, culls backward pointing portals, tomorrow add clipping planes formed by portal edge and camera)
* Investigate multiple passes (shadows, lights)
