# Monster-mouth escape art

The runtime art set for the monster-mouth escape game: every file in this folder, its pixel size, and what it depicts.

[`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes every PNG here from the generations listed in [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records each generation's tool, model, style and prompt. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v1.png) shows the set beside the concept images.

## Use

- No file here is loaded by the game yet. The empty `.gdignore` keeps Godot from importing this folder; the change that loads the art deletes it and commits the `.import` files Godot then writes.
- The map grid is 36×16 cells of 32 px on a 1280×720 screen. World tiles are one cell; larger sprites list their size in cells.
- Every sprite is drawn at 1:1 screen pixels. Scale 2 marks art drawn on a 2×2 pixel grid, so it stays crisp at its listed size; draw it at that size, not doubled again.
- Transparent PNGs have only fully opaque or fully clear pixels. Sample every PNG with nearest filtering.
- Facing: characters face right; flip horizontally for left.
- No image contains text except `title/logo.png`. The engine draws every word and number.

## Backgrounds

| File | Size | Scale | Depicts |
|---|---|---|---|
| `backgrounds/throat.png` | 1280×720 | 2 | Back layer: the monster's upper jaw with fangs and the pink throat; opaque |
| `backgrounds/city.png` | 1280×720 | 2 | Middle layer: indigo skyline with orange windows and an elevated train; clear above the skyline |
| `backgrounds/acid.png` | 1280×492 | 2 | Front layer: green acid with sunken train cars. The surface is near the top edge; move the layer so that edge follows the acid level, and fill any screen area below it with its bottom row |

## Characters

48×48 each, scale 1, standing on the bottom edge. The figure is about 32 px wide, so it overlaps its cell's neighbours a little.

| File | Depicts |
|---|---|
| `characters/chef_idle.png` | Chef (ART-CHAR-001) standing with the spatula |
| `characters/chef_walk_1.png`, `characters/chef_walk_2.png` | Chef walking, two frames |
| `characters/chef_climb.png` | Chef climbing, seen from behind |
| `characters/appraiser_idle.png` | Appraiser (ART-CHAR-002) standing with the magnifier |
| `characters/appraiser_walk_1.png`, `characters/appraiser_walk_2.png` | Appraiser walking, two frames |
| `characters/appraiser_climb.png` | Appraiser climbing, seen from behind |

## World

All scale 1.

| File | Size | Cells | Depicts |
|---|---|---|---|
| `world/platform.png` | 32×32 | 1×1 | Walkway deck: riveted iron beam with an orange top rail; repeats horizontally |
| `world/ladder.png` | 32×32 | 1×1 | Ladder with two rungs; repeats vertically |
| `world/ladder_broken.png` | 32×64 | 1×2 | Broken ladder: snapped rails and missing rungs |
| `world/door_cold_store.png` | 32×64 | 1×2 | Cold store door: riveted steel with a frosted window |
| `world/door_broken.png` | 32×64 | 1×2 | Smashed, boarded door |
| `world/building_wall.png` | 64×64 | 2×2 | Building body: indigo concrete with lit and dark windows; repeats in both directions |
| `world/vending_machine.png` | 40×48 | about 1×1.5 | Broken vending machine with jars behind cracked glass |
| `world/shop.png` | 96×96 | 3×3 | Corner shop front with shelves, crates, awning and lantern |
| `world/vendor_cart.png` | 40×64 | about 1×2 | Street stall: noodle cart with canopy and lantern |
| `world/noodle_vendor.png` | 26×48 | about 1×1.5 | Noodle vendor NPC holding a ladle |
| `world/kiosk.png` | 64×64 | 2×2 | Wrecked kiosk with flesh slime on the roof |
| `world/bridge.png` | 32×32 | 1×1 | Rotten bridge: hanging planks on a chain; repeats horizontally |
| `world/ferry.png` | 192×112 | 6×3.5 | Drifted ferry: rusty hull, lit cabin, lantern |
| `world/bag_washed_up.png` | 32×52 | 1×1.6 | Washed-up bag: slimy backpack with a sparkle above it |
| `world/bag_roof.png` | 32×34 | 1×1 | Roof bag: roped duffel |
| `world/cold_store.png` | 128×128 | 4×4 | Cold store block in scaffolding with a steel door and lamp |
| `world/acid_drip.png` | 32×96 | 1×3 | Acid drip hazard: flesh blob, falling drops, splash |

## Icons

48×48 each, scale 1, for item slots.

| File | Depicts |
|---|---|
| `icons/ingredient_tomato.png` | Tomato |
| `icons/ingredient_fish.png` | Fish |
| `icons/ingredient_kelp.png` | Kelp |
| `icons/ingredient_mushroom.png` | Mushroom |
| `icons/ingredient_pickles.png` | Pickles |
| `icons/ingredient_meat.png` | Meat |
| `icons/ingredient_truffle.png` | Truffle |
| `icons/tool_magnifier.png` | Magnifier |
| `icons/tool_spatula.png` | Spatula |
| `icons/tool_wrench.png` | Wrench |
| `icons/dish_mushroom_soup.png` | Mushroom Soup |
| `icons/dish_fish_stew.png` | Fish Stew |
| `icons/dish_kelp_wrap.png` | Kelp Wrap |
| `icons/dish_meat_skewer.png` | Meat Skewer |
| `icons/dish_truffle_roast.png` | Truffle Roast |
| `icons/dish_pickle_surprise.png` | Pickle Surprise |
| `icons/dish_sea_salad.png` | Sea Salad |
| `icons/dish_fish_pie.png` | Fish Pie |
| `icons/dish_meat_stew.png` | Meat Stew |
| `icons/dish_truffle_soup.png` | Truffle Soup |
| `icons/dish_surf_and_turf.png` | Surf and Turf |

## Workshop

| File | Size | Scale | Depicts |
|---|---|---|---|
| `workshop/kitchen.png` | 1280×720 | 2 | Empty base kitchen: shelves, lanterns, stove, cauldron, workbench, and the feeding crane over the acid; opaque |

## UI

All scale 2. The frames have a clear margin of 2 to 6 px around the drawn border; the 9-slice margins are the integrator's choice.

| File | Size | Depicts |
|---|---|---|
| `ui/panel_frame.png` | 96×96 | Riveted metal panel frame with a dark inside, for 9-slice |
| `ui/bar_frame.png` | 192×72 | Riveted bar frame with an empty inside; the fill is drawn by the engine |
| `ui/item_slot.png` | 64×64 | Item slot frame for a 48 px icon |
| `ui/heart_full.png` | 32×32 | Heart, full |
| `ui/heart_empty.png` | 32×32 | Heart, empty |
| `ui/button.png` | 64×64 | Button, normal, blank face |
| `ui/button_highlight.png` | 64×64 | Button, highlighted, blank face |
| `ui/icon_day.png` | 32×32 | Day: sun |
| `ui/icon_clock.png` | 32×32 | Clock |
| `ui/icon_acid.png` | 32×32 | Acid: green drop |

## Title

| File | Size | Scale | Depicts |
|---|---|---|---|
| `title/key_art.png` | 1280×720 | 2 | Key art: monster mouth, swallowed city, acid, the chef and the appraiser on a scaffold; opaque, with room for the logo in the upper middle |
| `title/logo.png` | 512×512 | 2 | Logo: MONSTER MOUTH ESCAPE lettering with fangs and acid drips; transparent |
