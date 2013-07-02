- Press "o" to open the settings window, changes to values here are immediate.

- After startup, you should load the defaults using shift-u (the values in the settings window should change). Changes to these settings can be changed with command-u (always into the same file, boost_parameters_).

- game_state allows to switch between editor and playing, the difference is the lack of handles on non-user editable objects. By default, only temperature changing objects (Brownian_box) are user-editable.

- To add a defined game field, edit any "game_field_*" value. The depth (front and back) should not be changed.

- To add objects, choose the type in the "particle type" drop down box and command-click in the view.

- All objects added are persistent but molecules and Brownian_box-es. Molecules are supposed to be released by the Molecule_releaser.

- Select an object by left-clicking the any handle and press "Del" to delete it.

- Right-click any handle of an object to edit specific properties. Click outside of the menu or ESC to close it (only then the changes are taken into account).

- To try a level, press "Start Level". This will reset the objects (delete molecules, reset releasers etc.). There is no need to set the game_state to "Playing".

- "Toggle simulation" pauses the game.

- Clear deletes all objects. 

- "Save/Load state" save/load the level without non-persistent objects.