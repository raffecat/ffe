Authors

Must not violate the rules of the game:
* move item from one container to another
* exchange currency for new item
* exchange item for currency
* take items and complete quest, giving rewards
* advance quest and give or take items

Either make all of these things a single atomic command,
or make groups of commands (between yields) atomic.

Avoid reserving words that players might use,
but use words that read well as instructions.

Use 'set to' instead of 'put in' for setting variables and properties.
Use 'remember' to save things in actor memory.
Use repeat with (name) = (lower) to (upper)
Use for each (name) in (container)

Make some NPCs and Quests using mob commands, to see how they work.
The bones NPC.
The trash collector.
Indiana Jones temples.
The scroll ghosts temple.
Swim to bottom of fountain; swim through hole; etc.

Some things authors need to be able to do:
give new radakara.items.fae.sword to player



get {item from room of player or player}:
    find {thing} in room of player or in inventory of player or in equipment of player as item
    if item does not exist then fail There is no {thing} here.
    if item is not in room of player then fail You already have the {item}.
    send take to item

get {item from container} from {container from player or room of player}:
    find {place} in inventory of player or in equipment of player or in room of player as container
    if container does not exist then fail There is no {place} here.
    find {thing} in container as item
    if item does not exist then fail There is no {thing} in the {container}.
    send takeFrom to container

put {item from player} in {container from player or room of player}:
    find thing in inventory of player or in equipment of player or in room of player as item
    if item does not exist then fail There is no {thing} here.
    if item is not in inventory of player then fail You do not have the {item}.
    find place in inventory of player or in equipment of player or in room of player as container
    if container does not exist then fail There is no {place} here.
    if item is in equpment of player then fail You are wearing the {item}.
    send placeIn to container

wear {item from player or room of player}:
  if item is not in player then fail You do not have the {item}.
  if player is wearing item then fail You are already wearing the {item}.
  if player cannot wear item then fail You cannot wear the {item}.

-> is_wearing item = item is in this and worn of item is not empty
-> can_wear item = slots of this contains slot of item


-- different reactions to 'take' message

on take:
    fail The item is fastened to the wall.

on take:
    echo As you pick up the statue, you hear a subtle click.
    begin trap-triggered in room temple.traproom

on take:
    echo As you try to pick up the sword, it disintegrates into dust.
    action As {player} tries to pick up the sword, it disintegrates into dust.
    dest me
    done -- end of activity; succeeded


-- in the item prototype

on take:
    move this to player
    echo You pick up {this}.
    act {player} picks up {this}.
    trigger take player this

on drop:
    move this to room of player
    echo You drop {this}.
    act {player} drops {this}.
    trigger drop player this

on wear:
    if this is in equipment of player then
        done You are already wearing {this}.
    end if
    if slot of this is not in slots of player then
        fail You cannot wear {this}.
    end if
    if there is an item in equipment of player with slot equal to slot of this then
        remove it
    end if
    move this to player
    echo You wear {this}.
    act {player} wears {this}.
    trigger wear player this

on remove:
    if this is not in equipment of player then
        done You are not wearing {this}.
    end if
    move this into inventory of player
    echo You remove {this}.
    act {player} removes {this}.
    trigger remove player this


-- convenience verbs

on action {number?} {text}
    -- this needs to do actor subtlety checks
    repeat for each observer in room of this
        send msg {text} to observer
    end repeat

on trigger {act} {who} {what}
    -- this needs to do actor subtlety checks
    repeat for each observer in room of this
        send {act} {who} {what} to observer
    end repeat

on echo {text}
    send msg {text} to player

on msg {text}
    append {text} to connection of player
