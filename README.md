# Team05-Lord-of-the-Lakes
## M4 Outline
> For M4, we worked towards the end of our game. More dialogue was added and drawings were inserted for a few story moments.
> The fish catch rates, pricing, stats were adjusted to feel like progression is smooth. Battling stats were also balanced so that enemies were not too strong if the player was properly upgraded.
> More sound effects and visual effects were added to the battle system to make it feel more exciting.
> A boss fight was implemented for the first lake. When the player defeats it, they are given access to a new lake implemented as well, which features another ally to recruit and a different fish probability pool
> Unforunately, we were not able to implement the ending of our game - currently, our game simply goes up to recruiting ally 2.

## User Experience
- Some UX was done in M3 already from feedback (see QOL and Misc. changes in M3 milestone)
- Battle system was changed to use the mouse instead of keyboard to select skills since some people expressed confusion (why don't skills trigger with click when skills can be hovered with mouse)
- Party member descriptions added to the party menu - most importantly the skills descriptions are added there, hopefully for easier reading to allow players to understand what the skills are doing when viewing them outside of battle
- Shiny spots effects were increased to motivate player to move around the lake

## Game Balancing
- Fish monetary value and shop upgrade value changed to enhance difficulty (price increased), but rarity of fish adjusted to get better fish and better valued fish as a balance.
- Lures added to increase catch rate for faster fishing
- Battling balanced from testing - the regular spawned fish should be difficult to defeat without having upgraded anything, and most likely impossible without having first recruited the ally
- However, once players have bought a couple of upgrades, these battles should get easier.
- Boss balanced so that it is possible to win when minimum requirements are met (7 upgrades of each shop upgrade), but also possible to lose if player doesn't apply the turn based skills "correctly"
- We decided to make it on the side of having an easier difficulty so that players should not feel frustrated because of grinding. Shop upgrades max limits were increased so if players wanted to keep grinding, they could buy more upgrades as a reward.
- The fights are still winnable without neediing excessive upgrades.
- MC heal was buffed to feel less like a "wasted turn" and Ally 1's debuff skill's duration was buffed so it did not need to be constantly reapplied

## Comprehensive Tutorial
- On starting the game, after reading through the intro cutscene, the tutorial screen is the first thing the player sees.
- The player should click on these tutorials to understand the mechanics of the game
- If player does not look at them and continues after saving, the tutorials are presented to the player.
- When hitting a fish shadow for the first time, a battle tutorial will pop up and the player must go through a slideshow that explains the battle mechanics
- When hitting a shiny spot for the first time, a small dialogue is triggered and explains the effect of the shiny spots.
	- world_system.cpp (handle_collision)
	- render_system.cpp (battle tutorial, handles what screen is shown on start up and continue (also saved through world system)

## Interpolation!!
	- The salmon is "thrown" using interpolation, during an attack by the boss
		- render_system.cpp (ENEMY_ATK)

## Particle Effect
	- Particle effect is used on the title screen
		- render_system (START_MENU)
	- A semi-particle effect is used for ally 1's attacks
		- battle_system.cpp (createParticles)

## Simple rendering effect
	- When player is colliding with a shiny spot, the player has a glow effect
		- done in drawSpriteAnime() in render_system.cpp (lightUp)

## More Story
	- More dialogue cutscenes were added
	- Images added to ally recruitments
		- dialogue.json
		- Introduction (render_system (new game) "introduction")
		- Boss appearance ( world_system "boss_appear")
		- Boss win (i.e. defeated the boss succesfully, world_system "boss_win")
		- Lake 2 entry (world_system "lake_2_entry")
		- Ally 2 recruitment (fish him up once in lake 2)
		- Ally 2 talk (party menu - talk)
		- Ally 1 new talk (party menu - talk after defeating the first boss)



## M3 Outline
> For M3, we mainly improved upon our existing features as well as adding some dialogue/story elements to introduce party members into our gameplay.
> More fish were added to the database for the player to catch and turn based battles have different enemies as well to fight.
> The battles also have more interesting mechanics to choose when fighting a fish, in addition to having a new party member joining the fight.
> Reloadability was implemented so that player position, stats, flags, party members and fish are saved and can be reloaded at any point from the last save.
> Shiny spots were also added to the lake where players can move around in the lake to find a spot and increase their catch rate, and lures can also be bought as a one-time bonus to catch rate.
> Some missing features that we expected to implement were to have more fishing minigames and an additional location to move to where new fish could be caught.
> In the last stretch of M4, we hope to further improve fishing gameplay, add bosses, and write a simple story to complement the game.

## Reloadability
	- Clicking the save option opens a prompt to save the game. It can also be saved from the settings menu, where the player can also load the game from as well.
	- A basic start menu was created as well to either start a new game or load an existing save.
		- render_system.cpp
		- saveData()
		- save and load textures, and the start menu, are also implemented in render_system.cpp

## Dialogue Cutscenes
	- When the player has caught a certain number of fish, the next time the player catches a fish, the party member recruitment is triggered
	- A short dialogue can also be triggered when pressing the talk button on the party menu
		- Event triggered in world_system.cpp (catch_fish(), when flags and conditions are met)
		- Dialogue rendering done in render_system.cpp 
		- drawDialogueCutscene()

## Battle Improvements
	- More assets and animation functions were implemented to improve the battle system
	- Fish are caught when a fish is defeated by the battle gameplay
	- Skills are created when initiating a party member. These skills can have additional effects that change ally and enemy stats.
		- skill made in common.cpp
		- battle_system.cpp
		- render_system.cpp

## More Asset creations
	- More fish added to the database
	- Effect sprites added to battle
	- Talk sprites added to dialogue
	- New lake texture added \

## QOL and Misc. changes
	- A new font was added for readability, font size increased
	- Visual and audio cues added in the Shop to let player know if they have bought or cannot buy something
	- Hover text added in battle to help understand skill mechanics.
	- Fish shadows do not spawn near player on starting the game. Also, shadows will respawn after a short period of time.

## M2 Outline
> During the course of development, we ended up having our hands full with working on the UI and understanding the Dear ImGui library. 
> We have succesfully drawn up most of the base UI templates that can be linked to registry to display data, and will be further developed
> and polished as we continue working on the game. A basic fishing minigame was made with a visual bar that fills up by mouse clicks, and the gameplay
> is timed with our sprite sheet animations and other assets, plus a simple sound system. A tutorial that outlines the controls of our game was also made.
> Turn based combat system has been implemented. Most of our milestone plans were worked in, however, the party member features and story based features
> were not able to touch on yet. Since we have implemented a lot of foundational systems, we believe we will be able to catch up on the missing features
> in M2 as well as completing the planned features of M3 in the following weeks.

## Game logic response to user input
### Battle System
	- On collide with a fish shadow in the world, a turn-based combat begins (or press B)
		- battle_system.cpp
		- UI and assets drawn in render_system.cpp::drawBattle()

### Fishing Minigame
	- When pressing "F", a collision is triggered and a fishing timer starts. When the fishing timer ends, an exclamation mark appears.
	- On click during the exclamation mark phase proceeds to the catching bar phase, where user must click until the bar is filled to catch a fish
		- world_system.cpp
		- A combination of programming in step(), handle_collisions(), on_key(), on_mouse_button(), and some great boolean work.

### Shop, Money, and Inventory
	- After catching a fish, the fish will appear in the inventory. It will also be available to sell in the shop
	- Once catching a species of fish, the species is "unlocked" in the fishing log
	- Gold can be spent on basic upgrades that updates the fishingRod component properties
		- render_system.cpp, drawInventory() and drawShop()

## Sprite sheet animation
### Animation System and Sprite Sheets
	- An animation system was made for entities that will go through set sprite frames based on Sprite component
		- animation_system.cpp/hpp
	- Texture sampling is done in a function in render_system.cpp
		- DrawSpriteAnime() in render_system.cpp
	- Sprite sheets made for the player character (player_<direction>_sheet) and the fishing rod during turn based battles (rod_idle and rod_reel)

## New integrated assets + Basic user tutorial/help
### Fish Data
	- Added more species of fish
		- common.hpp, starting from line 80

### Dear ImGui Inventory, Shop, Party, Tutorial and Battle UI
	- Using combination of the Dear ImGui library and texture assets, windows and text are displayed based on game state 
	- Pressing M, O, P, Esc, and B brings up each UI respectively
		- render_system.cpp (There's a lot)

### Sound System
	- Created simple sound system that plays sound effects and bgm
		- sound_system.cpp/hpp
		- credits for audio can be found at bottom of README

## Stability
### Mesh collision and Camera
	- Player collides with lake boundary mesh in a visually satisfying way. (line 81 - 127)
	- Camera follows player (lines 144 - 158)
		- physics_system.cpp

### Consistent screen resolution
	- Window creation takes in the user's monitor dimensions and changes the screen on open based on those dimensions. (The height is a little shorter than work area to take into account OS UI such as taskbars)
	- Scaled GUI 
	- Keeps aspect ratio of 5:3
		- world_system.cpp create_window()
		- scaling done in render_system(), the scale is set in render_system_init.cpp :: init()

## M1 Outline
> We were able to implement almost all of the features we had planned for this milestone, including the camera positioning and setting up foundations of our fishing gameplay. One feature we planned but was not able to implement fully was making a rudimentary inventory UI and textbox popups. We will be expanding on UI in M2, so we are still on track with our current proposal plans.

### Textured Geometry
	- New textures added into the data/textures folder. 
	- Used for the player, the fish shadows and the lake and land areas of the game
	- IDs created in components.hpp
		- world_system.cpp - OnKey function for different orientations of player

### Basic 2D Transformations
	- Position translation of the player character and fish shadows
		- physics_system.cpp
		- render_system.cpp
		- world_system.cpp

### Key-frame Interpolation
	- Smooth movement per step of moving entities (Using velocity to translate position per step_second)
		- physics_system.cpp - step function

### Keyboard-Mouse Control
	- Player moves with WASD keys. Can also trigger a fishing event with the F key
		- world_system.cpp - OnKey function

### Random Coded Action
	- Fish shadows will move randomly every 2.5 seconds based on an associated timer 
	- Currently have two types of fish that can be caught - fish and carp - that are randomly added to player after pressing F
		- world_system.cpp - step function

### Well-defined game-space boundaries
	- World is made up of a textured geometry representing a lake and one that represents the land area
	- Vertices are created in
		- render_system_init.cpp

### Correct Collision Processing
	- Player cannot move past the lake area - collides with edges of lake
	- Player will bump if hitting a fish shadow
	- Fishing collides with water to trigger a fishing event. If fishing ends up landing in a land area for some reason, it does not trigger the event.
		- physics_system.cpp
		- world_system.cpp

### Dear ImGUI Integration
	- Library can be found in the /imgui folder
	- A test window was created within the game
		- Initiated and deconstructed in main.cpp
		- Rendered in render_system.cpp

### Camera Positioning
	- Camera moves with player so that player does not move offscreen.
	- Camera bound by game-space boundaries
	- Manipulates glViewPort to move camera
		- render_system.cpp - lines 140-145

References:
---
SFX:
- [Catch fish splash](https://freesound.org/people/Fission9/sounds/583348/)
- [Rod swing](https://freesound.org/people/InspectorJ/sounds/394429/)
- [Start fish splash](https://freesound.org/people/swordofkings128/sounds/398032/)
- [Exclamation alert](https://freesound.org/people/aj_heels/sounds/634079/)
- [Cash register "Cha-ching!"](https://freesound.org/people/Zott820/sounds/209578/)
- [SFX for denied UI and selling](https://leohpaz.itch.io/rpg-essentials-sfx-free)
- [Sound effect Starter pack (over 500 sound)](https://simon13666.itch.io/sound-starter-pack)

BGM:
- http://www.abstractionmusic.com/

FONT:
- https://ggbot.itch.io/bad-comic-font
- https://www.ffonts.net/Carre.font.download
