# ADR 0013: Click what the screen shows as the key it stands for

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

Every command was a key, and the screen named each one, so a player had to learn about twenty keys per player before playing ([#154](https://github.com/garamsh/neocity/issues/154)). The owner asked for movement on the keyboard and every other interaction by mouse, without adding shortcuts. The rules (`src/survival/**`) take `PlayerInput` from session, so a click can reach them only as the same input a key gives. Two players share one machine, and therefore one mouse, while each has a key set. The C++ host has no Godot classes for viewports or controls, so input reads the mouse through what it already has.

## Decision

A click on something the screen draws presses the key that thing stands for, and nothing else: a menu entry presses its number key, a human's action button presses that human's interact key, the task card's step presses the key it names, and so on. The HUD lists the clickable things of each screen with the press each stands for and, while it would do nothing, the reason. Session turns a click into those presses on the frame's `Command` before the screen advances, so a click and its key take the same path through session and the rules. The keys all stay.

With two players and one mouse, the screen names the actor and the mouse does not choose one:

- At base the role decides: cooking, feeding and ending the day are the chef's, appraising the appraiser's. The set-out step sets out every human at base.
- In the city each human's own action button and travel key are the only targets. A click on a source on the map does nothing.

The rejected alternative was to let a click on a source on the map act for the human nearest it. It needs the map's camera in the HUD and a tie rule for two humans the same distance away, and a player clicking a source cannot tell from the screen which human will act.

## Consequences

- Every command reachable by mouse is also reachable by key, and the key checks keep proving the commands themselves. A mouse check only has to prove that a click reaches the right key.
- The HUD's painting and its targets share one source for each area, the rise of a panel coming into place included, so a moved button moves its target with it.
- A target that refuses shows its reason on hover. A click on it presses nothing.
- The city cannot be played from the map. A player always clicks a button in the action panel.

## Signs the decision was wrong

- Players try to click sources and characters on the map, and the action panel goes unnoticed.
- A command needs a choice the keys never offered, such as cooking a dish named by its recipe. Then a click is no longer one key press, and the mapping in session needs a command of its own.
- Two players at one machine fight over the mouse for city actions, and each would rather have a click that acts for the human nearest to it.
