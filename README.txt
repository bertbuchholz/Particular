Level Editing
-------------

- Press "o" to open the editor mode window.

- Switch to "Level editor" mode under "Interface"

- The size of the bounding box in which the molecules can move around
  can be changed with the "Game Field" values, the depth should be kept
  at 40 or lower for playable levels.  If you change the depth, you
  might need to re-add objects already placed so they have the correct
  depth.

- To add objects, choose the type in the "Object Type" drop-down box and
  use command + left-click to place objects.  The handles can be used to
  change position, rotation and size.  Right-clicking one of the
  handles allows to change the object's parameters (if any).

- Objects (these are the tested ones, there are others though):

  Molecular_releaser: release molecules over time in the direction
  defined by the arrow, right-click menu allows to change the type of
  the molecule, the interval, number and start of the release.

  Box_barrier: Blocks (repels) molecules

  Portal (Sphere and Box): Goal for the molecules, can be either set to
  destroy molecules that enter or keep them alive (with the possibility
  that the molecules leave the portal again).

  Player-changeable objects:

  Tractor_barrier: Push/pull molecules, speed, range and direction can
  be set during playing.

  Brownian_box: Create hot/cold areas, hot areas introduce brownian
  motion in the molecules (they start moving randomly).  Cold areas will
  simply remove otherwise created heat, but will not add additional
  damping.

- Added objects are by default persistent except for player-changeable
  ones. All object properties can be made available to or taken away
  from the player by checking the options in the right-click menu. So it
  is possible to restrict the size of a Brownian_box to the default.

- Select an object by left-clicking any handle and press "Delete" to
  delete it.

- To try a level, press "Reset Level" and toggle the simulation. This
  will also reset the objects (delete molecules, reset releasers etc.).

- "Toggle simulation" pauses the game.

- Clear deletes all objects.

- General settings per level (under Level_data and Core):

  Gravity: adds a constant downward force

  Damping: Damps translation/rotation

  Fluctuation: Externally applied random brownian motion (i.e.,
  temperature). The higher the value, the stronger molecules will start
  moving

  Available elements: The elements and their respective quantities the
  user can use while playing a level.  In most cases, this should be
  Tractor_barrier and Brownian_box

- Save/load a level to the folder data/levels

- To change the levels that are presented to the player in the normal
  playing mode, add its name to the topmost string entry "levels".  Only
  give the actual level name, not the file name.  E.g., when the
  filename is "level1.data", add "level1" to the string. It's a
  comma-separated list, no spaces between commas and words.

- "Save/Load settings" saves the general settings that will be applied
  at the start of the game. The defaults should be fine, but it can be
  interesting to play around with them, especially the molecular forces
  (Atomic Force Type).
